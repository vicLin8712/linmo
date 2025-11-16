#!/usr/bin/env python3
"""
Automate scheduler comparison runs (OLD vs NEW).

- 固定跑一組 scenarios（SCENARIOS）
- 每個 scenario 重複跑 REPEAT 次
- 比較 OLD / NEW 排程器平均排程時間
- 收集 improvement_ratio = old_avg_ns / new_avg_ns
- 對每個 scenario 輸出：
    * mean improvement
    * std dev of improvement
    * min / max improvement
    * 95% CI of improvement
    * mean old / new sched time
    * max old / new sched time
- 輸出兩張圖：
    1. bench_improvement.png
       y 軸：Δx = improvement_ratio - 1
    2. bench_new_sched_time.png
       y 軸：new scheduler avg sched time (ns)
"""

from __future__ import annotations
import os
import signal
import subprocess as sp
import re
import statistics
import math

import matplotlib.pyplot as plt  # pip3 install matplotlib
from matplotlib.ticker import MaxNLocator

END_TOKEN = "END TEST"

# ======== 可調整設定 ========
SCENARIOS = [0, 1, 2, 3, 4]  # 對應 C 裡的測試情境
REPEAT = 20  # 每個 scenario 重複次數
MAKE_TARGET = "sched_cmp"  # build 用的 make target
RUN_TARGET = "run"  # 執行用的 make target
WORKDIR = "."  # 專案根目錄（含 Makefile）
MAKE_VARS: dict[str, str] = {}  # 額外傳給 make 的變數
NO_CLEAN = False  # True 則不跑 make clean

# x 軸括號要標的「負載 %」，依 scenario 順序對應
SCENARIO_LOAD_PERCENTS = [2, 4, 20, 50, 100]
# ============================

AVG_RE = re.compile(
    r"(?i)(old|new)\s+scheduler\s+avg\s+schedul\w*\s+time:\s*([0-9]+(?:\.[0-9]+)?)\s*(μs|us|microseconds?|ns|nanoseconds?)"
)
MAX_RE = re.compile(
    r"(?i)max(?:imum)?\s+schedul\w*\s+time:\s*([0-9]+(?:\.[0-9]+)?)\s*(μs|us|microseconds?|ns|nanoseconds?)"
)
TESTNAME_RE = re.compile(r"Running test:\s*(.*?)\s*for", re.IGNORECASE)
TASKCNT_RE = re.compile(r"Task count:\s*(\d+)", re.IGNORECASE)
ACTRATIO_RE = re.compile(r"Task active ratio:\s*(\d+)", re.IGNORECASE)


def _to_ns(val: float, unit: str) -> float:
    unit = unit.lower()
    if unit.startswith(("μ", "u", "micro")):
        return val * 1000.0  # μs -> ns
    return val  # 默認 ns


def build(
    target: str,
    scenario: int,
    old: bool,
    make_vars: dict[str, str],
    clean: bool,
    cwd: str | None,
):
    """Rebuild target with scenario + OLD/NEW flags."""
    if clean:
        sp.run(["make", "clean"], cwd=cwd, stdout=sp.DEVNULL, stderr=sp.DEVNULL)
    vars_list = [f"{k}={v}" for k, v in make_vars.items()]
    vars_list.append(f"TEST_SCENARIO=-DTEST_SCENARIO={scenario}")
    if old:
        vars_list.append("TEST_FLAGS=-DOLD")

    cmd = ["make", target] + vars_list
    sp.run(cmd, cwd=cwd, stdout=sp.DEVNULL, stderr=sp.DEVNULL, check=True)


def run_and_capture(
    run_target: str, cwd: str | None
) -> tuple[float, float, str, int, int | None, list[int] | None]:
    """
    Run test via make run, capture:
      - avg scheduling time (ms)
      - max scheduling time (ms)
      - test name
      - task count
      - active ratio (optional; may be None)
      - task distribution [0..7] (optional; may be None, 尚未實作)
    """
    avg_time_ns: float | None = None
    max_time_ns: float | None = None
    test_name: str | None = None
    task_count: int | None = None
    active_ratio: int | None = None
    distribution: list[int] | None = None

    proc = sp.Popen(
        ["make", run_target],
        cwd=cwd,
        stdout=sp.PIPE,
        stderr=sp.STDOUT,
        text=True,
        bufsize=1,
        universal_newlines=True,
        preexec_fn=os.setsid if hasattr(os, "setsid") else None,
    )

    try:
        assert proc.stdout is not None
        for line in proc.stdout:
            if not test_name:
                m_test = TESTNAME_RE.search(line)
                if m_test:
                    test_name = m_test.group(1).strip()

            m_avg = AVG_RE.search(line)
            if m_avg:
                val = float(m_avg.group(2))
                unit = m_avg.group(3)
                avg_time_ns = _to_ns(val, unit)

            m_max = MAX_RE.search(line)
            if m_max:
                val = float(m_max.group(1))
                unit = m_max.group(2)
                max_time_ns = _to_ns(val, unit)

            m_cnt = TASKCNT_RE.search(line)
            if m_cnt:
                task_count = int(m_cnt.group(1))

            m_ratio = ACTRATIO_RE.search(line)
            if m_ratio:
                active_ratio = int(m_ratio.group(1))

            if END_TOKEN in line:
                break

        try:
            if hasattr(os, "getpgid"):
                os.killpg(os.getpgid(proc.pid), signal.SIGINT)
            else:
                proc.terminate()
        except Exception:
            pass
        try:
            proc.wait(timeout=3)
        except Exception:
            try:
                os.killpg(os.getpgid(proc.pid), signal.SIGKILL)
            except Exception:
                pass
    finally:
        if proc.poll() is None:
            try:
                os.killpg(os.getpgid(proc.pid), signal.SIGKILL)
            except Exception:
                pass

    if avg_time_ns is None:
        raise RuntimeError("Failed to parse 'avg scheduling time' before END TEST.")
    if max_time_ns is None:
        max_time_ns = float("nan")
    if task_count is None:
        task_count = -1

    return (
        avg_time_ns,
        max_time_ns,
        (test_name or "Unknown Test"),
        task_count,
        active_ratio,
        distribution,
    )


def _fmt_factor(r: float) -> str:
    """格式化為 '1.12x' 這種字串。"""
    return f"{r:.2f}x"


def _fmt_factor_with_dir(r: float) -> str:
    """>1 表示 faster，<1 表示 slower。"""
    if r >= 1.0:
        return f"{r:.2f}x faster"
    else:
        return f"{r:.2f}x (slower than OLD)"


def _fmt_ns_and_us(ns_val: float) -> str:
    us = ns_val / 1000.0
    return f"{ns_val:.0f}ns ({us:.3f}μs)"


def main() -> None:
    proj = os.path.abspath(WORKDIR)
    print("Running scheduler benchmark tests: OLD vs NEW\n")
    print(f"Scenarios: {SCENARIOS}")
    print(f"Repeat per scenario: {REPEAT}\n")

    improvements_per_scenario: dict[str, list[float]] = {}

    # Store time statics
    old_avg_times: dict[str, list[float]] = {}
    new_avg_times: dict[str, list[float]] = {}
    old_max_times: dict[str, list[float]] = {}
    new_max_times: dict[str, list[float]] = {}

    for sc in SCENARIOS:
        scenario_improvements: list[float] = []
        scenario_label: str | None = None

        scenario_old_avgs: list[float] = []
        scenario_new_avgs: list[float] = []
        scenario_old_maxs: list[float] = []
        scenario_new_maxs: list[float] = []

        for run_idx in range(1, REPEAT + 1):
            # OLD
            build(
                MAKE_TARGET,
                sc,
                old=True,
                make_vars=MAKE_VARS,
                clean=not NO_CLEAN,
                cwd=proj,
            )
            (
                old_avg_ns,
                old_max_ns,
                test_name,
                task_cnt,
                act_ratio,
                dist,
            ) = run_and_capture(RUN_TARGET, cwd=proj)

            # NEW
            build(
                MAKE_TARGET,
                sc,
                old=False,
                make_vars=MAKE_VARS,
                clean=not NO_CLEAN,
                cwd=proj,
            )
            new_avg_ns, new_max_ns, _, _, _, _ = run_and_capture(RUN_TARGET, cwd=proj)

            diff_avg = old_avg_ns - new_avg_ns
            improvement_ratio = (
                old_avg_ns / new_avg_ns if new_avg_ns > 0 else float("inf")
            )

            scenario_improvements.append(improvement_ratio)
            scenario_label = test_name

            scenario_old_avgs.append(old_avg_ns)
            scenario_new_avgs.append(new_avg_ns)
            scenario_old_maxs.append(old_max_ns)
            scenario_new_maxs.append(new_max_ns)

            print(f"Scenario: {test_name} (id={sc}), run #{run_idx}")
            print(f"  Total task count: {task_cnt}")
            print(f"  Active task ratio: {act_ratio} %")
            if dist:
                dist_str = " ".join(str(x) for x in dist)
                print(f"  Priority distribution (0-7): {dist_str}")
            print(
                "  Old  avg sched time: {:s}  max sched time: {:.0f}us\n"
                "  New  avg sched time: {:s}  max sched time: {:.0f}us\n"
                "  Sched time difference: {:.0f}ns ({:.3f}μs)\n"
                "  Instant improvement : {:.2f}x faster than O(n)\n".format(
                    _fmt_ns_and_us(old_avg_ns),
                    old_max_ns,
                    _fmt_ns_and_us(new_avg_ns),
                    new_max_ns,
                    diff_avg,
                    diff_avg / 1000.0,
                    improvement_ratio,
                )
            )

        if scenario_label is None:
            scenario_label = f"Scenario {sc}"

        improvements_per_scenario[scenario_label] = scenario_improvements
        old_avg_times[scenario_label] = scenario_old_avgs
        new_avg_times[scenario_label] = scenario_new_avgs
        old_max_times[scenario_label] = scenario_old_maxs
        new_max_times[scenario_label] = scenario_new_maxs

    print("All tests finished.\n")

    if not improvements_per_scenario:
        print("No improvement data collected; skip stats and plotting.")
        return

    print("Per-scenario statistics (improvement vs OLD):\n")
    scenario_names = list(improvements_per_scenario.keys())
    mean_ratios: list[float] = []

    for name in scenario_names:
        ratios = improvements_per_scenario[name]
        n = len(ratios)
        mean_r = statistics.mean(ratios)
        mean_ratios.append(mean_r)

        std_r = statistics.stdev(ratios) if n > 1 else 0.0
        min_r = min(ratios)
        max_r = max(ratios)

        if n > 1:
            se = std_r / math.sqrt(n)
            ci_half = 1.96 * se
        else:
            ci_half = 0.0

        # Time statics
        o_avgs = old_avg_times[name]
        n_avgs = new_avg_times[name]
        o_maxs = old_max_times[name]
        n_maxs = new_max_times[name]

        mean_old_avg = statistics.mean(o_avgs)
        mean_new_avg = statistics.mean(n_avgs)
        max_old_time = max(o_maxs)
        max_new_time = max(n_maxs)

        print(
            f"Scenario '{name}':\n"
            f"  mean improvement        = {_fmt_factor_with_dir(mean_r)}\n"
            f"  std dev of improvement  = {_fmt_factor(std_r)}\n"
            f"  min / max improvement   = {_fmt_factor(min_r)}  /  {_fmt_factor(max_r)}\n"
            f"  95% CI of improvement   = [{_fmt_factor(mean_r - ci_half)}, {_fmt_factor(mean_r + ci_half)}]\n"
            f"  mean old sched time     = {mean_old_avg} us \n"
            f"  mean new sched time     = {mean_new_avg} us \n"
            f"  max  old sched time     = {max_old_time} us \n"
            f"  max  new sched time     = {max_new_time} us \n"
        )

    # ===== Fig 1：improvement =====
    x_positions = list(range(len(scenario_names)))

    plt.figure()
    for i, name in enumerate(scenario_names):
        ratios = improvements_per_scenario[name]
        deltas = [r - 1.0 for r in ratios]  # Δx
        xs = [i] * len(deltas)
        plt.scatter(xs, deltas, alpha=0.6, s=10)

        mean_delta = mean_ratios[i] - 1.0
        plt.scatter(i, mean_delta, marker="x", color="black", s=40)
        plt.text(
            i + 0.05,
            mean_delta,
            f"{mean_ratios[i]:.2f}x",
            ha="left",
            va="center",
            fontsize=8,
        )

    if len(SCENARIO_LOAD_PERCENTS) >= len(scenario_names):
        loads = SCENARIO_LOAD_PERCENTS[: len(scenario_names)]
    else:
        loads = [(i + 1) * 2 for i in range(len(scenario_names))]

    xtick_labels = [f"{name}\n({load}%)" for name, load in zip(scenario_names, loads)]
    plt.xticks(x_positions, xtick_labels, rotation=0, ha="center")

    ax = plt.gca()
    ax.yaxis.set_major_locator(MaxNLocator(nbins=10, integer=False))

    plt.ylabel("Improvement")
    plt.xlabel("Scenario (with load %)")
    plt.title(f"Scheduler Improvement (REPEAT={REPEAT}, 40s/run)")
    plt.tight_layout()
    plt.savefig("bench_improvement.png", dpi=150)
    print("Improvement plot saved to: bench_improvement.png")

    # ===== Fig2：New scheduler avg sched time (ns) =====
    plt.figure()
    for i, name in enumerate(scenario_names):
        avgs = new_avg_times[name]  # new scheduler avg schedule time (ns)
        xs = [i] * len(avgs)
        plt.scatter(xs, avgs, alpha=0.6, s=10)

        mean_new = statistics.mean(avgs)
        plt.scatter(i, mean_new, marker="x", color="black", s=40)
        plt.text(
            i + 0.05,
            mean_new,
            f"{mean_new:.0f}ns",
            ha="left",
            va="center",
            fontsize=8,
        )

    plt.xticks(x_positions, xtick_labels, rotation=0, ha="center")
    ax2 = plt.gca()
    ax2.yaxis.set_major_locator(MaxNLocator(nbins=10, integer=False))

    plt.ylabel("New scheduler avg sched time (ns)")
    plt.xlabel("Scenario (with load %)")
    plt.title(f"New scheduler avg scheduling time per run (REPEAT={REPEAT})")
    plt.tight_layout()
    plt.savefig("bench_new_sched_time.png", dpi=150)
    print("New-scheduler timing plot saved to: bench_new_sched_time.png")


if __name__ == "__main__":
    main()
