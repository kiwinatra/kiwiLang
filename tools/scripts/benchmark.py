#!/usr/bin/env python3

import argparse
import asyncio
import csv
import json
import matplotlib.pyplot as plt
import numpy as np
import os
import platform
import psutil
import re
import subprocess
import sys
import tempfile
import time
from datetime import datetime
from pathlib import Path
from typing import Dict, List, Optional, Tuple, Any
from dataclasses import dataclass, field
from concurrent.futures import ThreadPoolExecutor
import statistics

@dataclass
class BenchmarkResult:
    name: str
    iterations: int
    total_time: float
    avg_time: float
    min_time: float
    max_time: float
    std_dev: float
    memory_usage: float
    cpu_usage: float
    timestamp: datetime = field(default_factory=datetime.now)

@dataclass
class CompilationResult:
    name: str
    compile_time: float
    binary_size: int
    optimization_level: str
    success: bool
    error: Optional[str] = None

class BenchmarkRunner:
    def __init__(self, build_dir: Path, output_dir: Path):
        self.build_dir = build_dir
        self.output_dir = output_dir
        self.output_dir.mkdir(parents=True, exist_ok=True)
        
        self.results: List[BenchmarkResult] = []
        self.compilation_results: List[CompilationResult] = []
        
    def run_compiler_benchmarks(self):
        """Benchmark compiler performance"""
        print("Running compiler benchmarks...")
        
        benchmarks = [
            ("lexer", self._benchmark_lexer),
            ("parser", self._benchmark_parser),
            ("type_checker", self._benchmark_type_checker),
            ("codegen", self._benchmark_codegen),
            ("optimizer", self._benchmark_optimizer),
        ]
        
        for name, benchmark_func in benchmarks:
            try:
                result = benchmark_func()
                if result:
                    self.results.append(result)
                    print(f"  {name}: {result.avg_time:.3f}ms")
            except Exception as e:
                print(f"  {name} failed: {e}")
    
    def _benchmark_lexer(self) -> Optional[BenchmarkResult]:
        """Benchmark lexer performance"""
        test_file = self.build_dir.parent / "examples" / "large_program.kiwi"
        if not test_file.exists():
            test_file = self._create_test_file(10000)
        
        compiler_path = self.build_dir / "bin" / "kiwiLang"
        if not compiler_path.exists():
            compiler_path = self.build_dir / "kiwiLang"
        
        if not compiler_path.exists():
            return None
        
        times = []
        memory_usages = []
        cpu_usages = []
        
        for i in range(10):
            start_time = time.perf_counter()
            start_memory = psutil.Process().memory_info().rss / 1024 / 1024
            
            process = psutil.Popen(
                [str(compiler_path), "lex", str(test_file), "--no-output"],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE
            )
            
            cpu_percent = process.cpu_percent()
            _, _ = process.communicate()
            
            end_time = time.perf_counter()
            end_memory = psutil.Process().memory_info().rss / 1024 / 1024
            
            times.append((end_time - start_time) * 1000)
            memory_usages.append(end_memory - start_memory)
            cpu_usages.append(cpu_percent)
        
        return BenchmarkResult(
            name="lexer",
            iterations=10,
            total_time=sum(times),
            avg_time=statistics.mean(times),
            min_time=min(times),
            max_time=max(times),
            std_dev=statistics.stdev(times) if len(times) > 1 else 0,
            memory_usage=statistics.mean(memory_usages),
            cpu_usage=statistics.mean(cpu_usages)
        )
    
    def _benchmark_parser(self) -> Optional[BenchmarkResult]:
        """Benchmark parser performance"""
        test_file = self._create_nested_code(1000)
        
        compiler_path = self.build_dir / "bin" / "kiwiLang"
        if not compiler_path.exists():
            compiler_path = self.build_dir / "kiwiLang"
        
        if not compiler_path.exists():
            return None
        
        times = []
        
        for i in range(5):
            with tempfile.NamedTemporaryFile(mode='w', suffix='.kiwi', delete=False) as f:
                f.write(test_file)
                temp_path = Path(f.name)
            
            try:
                start_time = time.perf_counter()
                
                process = subprocess.run(
                    [str(compiler_path), "parse", str(temp_path), "--no-output"],
                    capture_output=True,
                    text=True
                )
                
                end_time = time.perf_counter()
                times.append((end_time - start_time) * 1000)
            finally:
                temp_path.unlink()
        
        return BenchmarkResult(
            name="parser",
            iterations=5,
            total_time=sum(times),
            avg_time=statistics.mean(times),
            min_time=min(times),
            max_time=max(times),
            std_dev=statistics.stdev(times) if len(times) > 1 else 0,
            memory_usage=0,
            cpu_usage=0
        )
    
    def _benchmark_type_checker(self) -> Optional[BenchmarkResult]:
        """Benchmark type checker performance"""
        test_file = self._create_typed_code(500)
        
        compiler_path = self.build_dir / "bin" / "kiwiLang"
        if not compiler_path.exists():
            compiler_path = self.build_dir / "kiwiLang"
        
        if not compiler_path.exists():
            return None
        
        times = []
        
        for i in range(3):
            with tempfile.NamedTemporaryFile(mode='w', suffix='.kiwi', delete=False) as f:
                f.write(test_file)
                temp_path = Path(f.name)
            
            try:
                start_time = time.perf_counter()
                
                process = subprocess.run(
                    [str(compiler_path), "typecheck", str(temp_path), "--no-output"],
                    capture_output=True,
                    text=True
                )
                
                end_time = time.perf_counter()
                times.append((end_time - start_time) * 1000)
            finally:
                temp_path.unlink()
        
        return BenchmarkResult(
            name="type_checker",
            iterations=3,
            total_time=sum(times),
            avg_time=statistics.mean(times),
            min_time=min(times),
            max_time=max(times),
            std_dev=statistics.stdev(times) if len(times) > 1 else 0,
            memory_usage=0,
            cpu_usage=0
        )
    
    def _benchmark_codegen(self) -> Optional[BenchmarkResult]:
        """Benchmark code generation performance"""
        simple_code = """
fn main() -> i32 {
    let mut sum = 0;
    for i in 0..1000000 {
        sum += i;
    }
    return sum;
}
"""
        
        compiler_path = self.build_dir / "bin" / "kiwiLang"
        if not compiler_path.exists():
            compiler_path = self.build_dir / "kiwiLang"
        
        if not compiler_path.exists():
            return None
        
        times = []
        
        for i in range(5):
            with tempfile.NamedTemporaryFile(mode='w', suffix='.kiwi', delete=False) as f:
                f.write(simple_code)
                temp_path = Path(f.name)
            
            try:
                start_time = time.perf_counter()
                
                process = subprocess.run(
                    [str(compiler_path), "compile", str(temp_path), "-o", "test_output"],
                    capture_output=True,
                    text=True
                )
                
                end_time = time.perf_counter()
                times.append((end_time - start_time) * 1000)
            finally:
                temp_path.unlink()
                output_path = Path("test_output")
                if output_path.exists():
                    output_path.unlink()
        
        return BenchmarkResult(
            name="codegen",
            iterations=5,
            total_time=sum(times),
            avg_time=statistics.mean(times),
            min_time=min(times),
            max_time=max(times),
            std_dev=statistics.stdev(times) if len(times) > 1 else 0,
            memory_usage=0,
            cpu_usage=0
        )
    
    def _benchmark_optimizer(self) -> Optional[BenchmarkResult]:
        """Benchmark optimizer performance"""
        loop_code = """
fn compute() -> i32 {
    let mut result = 0;
    for i in 0..1000 {
        for j in 0..1000 {
            result += i * j;
        }
    }
    return result;
}
"""
        
        compiler_path = self.build_dir / "bin" / "kiwiLang"
        if not compiler_path.exists():
            compiler_path = self.build_dir / "kiwiLang"
        
        if not compiler_path.exists():
            return None
        
        times_no_opt = []
        times_with_opt = []
        
        for i in range(3):
            with tempfile.NamedTemporaryFile(mode='w', suffix='.kiwi', delete=False) as f:
                f.write(loop_code)
                temp_path = Path(f.name)
            
            try:
                start_time = time.perf_counter()
                
                process = subprocess.run(
                    [str(compiler_path), "compile", str(temp_path), "-O0", "-o", "test_no_opt"],
                    capture_output=True,
                    text=True
                )
                
                end_time = time.perf_counter()
                times_no_opt.append((end_time - start_time) * 1000)
                
                start_time = time.perf_counter()
                
                process = subprocess.run(
                    [str(compiler_path), "compile", str(temp_path), "-O3", "-o", "test_with_opt"],
                    capture_output=True,
                    text=True
                )
                
                end_time = time.perf_counter()
                times_with_opt.append((end_time - start_time) * 1000)
            finally:
                temp_path.unlink()
                for output in ["test_no_opt", "test_with_opt"]:
                    output_path = Path(output)
                    if output_path.exists():
                        output_path.unlink()
        
        avg_no_opt = statistics.mean(times_no_opt)
        avg_with_opt = statistics.mean(times_with_opt)
        optimization_overhead = ((avg_with_opt - avg_no_opt) / avg_no_opt) * 100
        
        result = BenchmarkResult(
            name="optimizer",
            iterations=3,
            total_time=sum(times_with_opt),
            avg_time=avg_with_opt,
            min_time=min(times_with_opt),
            max_time=max(times_with_opt),
            std_dev=statistics.stdev(times_with_opt) if len(times_with_opt) > 1 else 0,
            memory_usage=0,
            cpu_usage=0
        )
        
        result.optimization_overhead = optimization_overhead
        return result
    
    def benchmark_execution(self, program_path: Path, iterations: int = 100) -> Optional[BenchmarkResult]:
        """Benchmark execution of a compiled program"""
        if not program_path.exists():
            return None
        
        times = []
        
        for i in range(iterations):
            start_time = time.perf_counter()
            
            process = subprocess.run(
                [str(program_path)],
                capture_output=True,
                text=True
            )
            
            end_time = time.perf_counter()
            times.append((end_time - start_time) * 1000)
        
        return BenchmarkResult(
            name=program_path.stem,
            iterations=iterations,
            total_time=sum(times),
            avg_time=statistics.mean(times),
            min_time=min(times),
            max_time=max(times),
            std_dev=statistics.stdev(times) if len(times) > 1 else 0,
            memory_usage=0,
            cpu_usage=0
        )
    
    def benchmark_compilation(self, source_file: Path, optimizations: List[str] = None) -> List[CompilationResult]:
        """Benchmark compilation with different optimization levels"""
        if optimizations is None:
            optimizations = ["-O0", "-O1", "-O2", "-O3", "-Os"]
        
        compiler_path = self.build_dir / "bin" / "kiwiLang"
        if not compiler_path.exists():
            compiler_path = self.build_dir / "kiwiLang"
        
        if not compiler_path.exists() or not source_file.exists():
            return []
        
        results = []
        
        for opt in optimizations:
            output_name = f"bench_{source_file.stem}_{opt.replace('-', '')}"
            
            start_time = time.perf_counter()
            
            try:
                process = subprocess.run(
                    [str(compiler_path), "compile", str(source_file), opt, "-o", output_name],
                    capture_output=True,
                    text=True,
                    timeout=30
                )
                
                end_time = time.perf_counter()
                compile_time = (end_time - start_time) * 1000
                
                binary_size = 0
                output_path = Path(output_name)
                if output_path.exists():
                    binary_size = output_path.stat().st_size
                    output_path.unlink()
                
                results.append(CompilationResult(
                    name=source_file.stem,
                    compile_time=compile_time,
                    binary_size=binary_size,
                    optimization_level=opt,
                    success=process.returncode == 0,
                    error=process.stderr if process.returncode != 0 else None
                ))
            except subprocess.TimeoutExpired:
                results.append(CompilationResult(
                    name=source_file.stem,
                    compile_time=30000,
                    binary_size=0,
                    optimization_level=opt,
                    success=False,
                    error="Timeout"
                ))
        
        return results
    
    def run_example_benchmarks(self):
        """Run benchmarks on example programs"""
        examples_dir = self.build_dir.parent / "examples"
        if not examples_dir.exists():
            return
        
        print("Running example benchmarks...")
        
        for example_file in examples_dir.rglob("*.kiwi"):
            if example_file.name.startswith("benchmark_") or "bench" in example_file.stem.lower():
                print(f"  Benchmarking {example_file.name}...")
                
                results = self.benchmark_compilation(example_file)
                self.compilation_results.extend(results)
                
                if results and results[-1].success:
                    compiled_path = Path(f"bench_{example_file.stem}_O3")
                    if compiled_path.exists():
                        exec_result = self.benchmark_execution(compiled_path, 10)
                        if exec_result:
                            self.results.append(exec_result)
                        compiled_path.unlink()
    
    def _create_test_file(self, lines: int) -> str:
        """Create a test file with many lines"""
        code_lines = []
        for i in range(lines):
            code_lines.append(f"let var{i} = {i} * 2;")
            code_lines.append(f"let str{i} = \"test string {i}\";")
            if i % 10 == 0:
                code_lines.append(f"fn func{i}() {{ return {i}; }}")
        
        return "\n".join(code_lines)
    
    def _create_nested_code(self, depth: int) -> str:
        """Create deeply nested code for parser testing"""
        code = "fn test() {\n"
        
        for i in range(depth):
            indent = "    " * (i + 1)
            code += f"{indent}if true {{\n"
        
        for i in range(depth, 0, -1):
            indent = "    " * i
            code += f"{indent}}}\n"
        
        code += "}\n"
        return code
    
    def _create_typed_code(self, count: int) -> str:
        """Create code with many type annotations"""
        code_lines = ["fn typed_test() -> i32 {"]
        
        for i in range(count):
            code_lines.append(f"    let x{i}: i32 = {i};")
            code_lines.append(f"    let y{i}: f64 = {i}.0;")
            code_lines.append(f"    let s{i}: string = \"test{i}\";")
        
        code_lines.append("    return 0;")
        code_lines.append("}")
        
        return "\n".join(code_lines)
    
    def save_results(self):
        """Save benchmark results to files"""
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        
        json_file = self.output_dir / f"benchmark_results_{timestamp}.json"
        csv_file = self.output_dir / f"benchmark_results_{timestamp}.csv"
        
        json_data = []
        for result in self.results:
            json_data.append({
                "name": result.name,
                "iterations": result.iterations,
                "total_time_ms": result.total_time,
                "avg_time_ms": result.avg_time,
                "min_time_ms": result.min_time,
                "max_time_ms": result.max_time,
                "std_dev_ms": result.std_dev,
                "memory_usage_mb": result.memory_usage,
                "cpu_usage_percent": result.cpu_usage,
                "timestamp": result.timestamp.isoformat()
            })
        
        json_file.write_text(json.dumps(json_data, indent=2))
        
        with open(csv_file, 'w', newline='') as f:
            writer = csv.writer(f)
            writer.writerow(["Name", "Iterations", "Avg Time (ms)", "Min Time (ms)", 
                           "Max Time (ms)", "Std Dev", "Memory (MB)", "CPU %", "Timestamp"])
            
            for result in self.results:
                writer.writerow([
                    result.name,
                    result.iterations,
                    f"{result.avg_time:.3f}",
                    f"{result.min_time:.3f}",
                    f"{result.max_time:.3f}",
                    f"{result.std_dev:.3f}",
                    f"{result.memory_usage:.2f}",
                    f"{result.cpu_usage:.1f}",
                    result.timestamp.isoformat()
                ])
        
        print(f"Results saved to {json_file} and {csv_file}")
    
    def generate_plots(self):
        """Generate plots from benchmark results"""
        if not self.results:
            return
        
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        plots_dir = self.output_dir / "plots"
        plots_dir.mkdir(exist_ok=True)
        
        names = [r.name for r in self.results]
        avg_times = [r.avg_time for r in self.results]
        min_times = [r.min_time for r in self.results]
        max_times = [r.max_time for r in self.results]
        
        plt.figure(figsize=(12, 6))
        
        x = np.arange(len(names))
        width = 0.25
        
        plt.bar(x - width, min_times, width, label='Min', color='lightgreen')
        plt.bar(x, avg_times, width, label='Average', color='skyblue')
        plt.bar(x + width, max_times, width, label='Max', color='lightcoral')
        
        plt.xlabel('Benchmark')
        plt.ylabel('Time (ms)')
        plt.title('Benchmark Results')
        plt.xticks(x, names, rotation=45)
        plt.legend()
        plt.tight_layout()
        
        plot_file = plots_dir / f"benchmark_times_{timestamp}.png"
        plt.savefig(plot_file, dpi=300)
        plt.close()
        
        if self.compilation_results:
            opt_levels = []
            compile_times = []
            binary_sizes = []
            
            for result in self.compilation_results:
                if result.success:
                    opt_levels.append(result.optimization_level)
                    compile_times.append(result.compile_time)
                    binary_sizes.append(result.binary_size / 1024)
            
            if opt_levels:
                plt.figure(figsize=(10, 8))
                
                plt.subplot(2, 1, 1)
                plt.bar(opt_levels, compile_times, color='cornflowerblue')
                plt.ylabel('Compile Time (ms)')
                plt.title('Compilation Time by Optimization Level')
                
                plt.subplot(2, 1, 2)
                plt.bar(opt_levels, binary_sizes, color='lightcoral')
                plt.ylabel('Binary Size (KB)')
                plt.xlabel('Optimization Level')
                plt.title('Binary Size by Optimization Level')
                
                plt.tight_layout()
                
                compile_plot = plots_dir / f"compilation_benchmark_{timestamp}.png"
                plt.savefig(compile_plot, dpi=300)
                plt.close()
        
        print(f"Plots saved to {plots_dir}")

def main():
    parser = argparse.ArgumentParser(description="Run kiwiLang benchmarks")
    parser.add_argument("--build-dir", type=Path, default=Path("build"),
                       help="Build directory containing compiler")
    parser.add_argument("--output-dir", type=Path, default=Path("benchmark_results"),
                       help="Output directory for results")
    parser.add_argument("--examples", action="store_true",
                       help="Run benchmarks on example programs")
    parser.add_argument("--compiler-only", action="store_true",
                       help="Only run compiler benchmarks")
    parser.add_argument("--plot", action="store_true",
                       help="Generate plots from results")
    parser.add_argument("--iterations", type=int, default=10,
                       help="Number of iterations per benchmark")
    
    args = parser.parse_args()
    
    if not args.build_dir.exists():
        print(f"Build directory not found: {args.build_dir}")
        return 1
    
    runner = BenchmarkRunner(args.build_dir, args.output_dir)
    
    print("Starting kiwiLang benchmarks...")
    print(f"System: {platform.system()} {platform.release()}")
    print(f"Processor: {platform.processor()}")
    print(f"Python: {platform.python_version()}")
    print()
    
    runner.run_compiler_benchmarks()
    
    if args.examples:
        runner.run_example_benchmarks()
    
    if not args.compiler_only and args.examples:
        print("Running execution benchmarks...")
    
    runner.save_results()
    
    if args.plot:
        runner.generate_plots()
    
    print("\nBenchmark summary:")
    print("-" * 60)
    print(f"{'Benchmark':<20} {'Avg Time (ms)':<15} {'Std Dev':<10} {'Memory (MB)':<12}")
    print("-" * 60)
    
    for result in runner.results:
        print(f"{result.name:<20} {result.avg_time:<15.3f} {result.std_dev:<10.3f} {result.memory_usage:<12.2f}")
    
    return 0

if __name__ == "__main__":
    sys.exit(main())