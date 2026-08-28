"""命令行工具入口.

子命令:
  search   关键词搜索 -> 清洗/对比/分析, 输出表格 + JSON
  report   search + 生成可视化图表 (PNG)
  sample   输出内置示例数据与结果 (无需联网, 供快速演示)
  web      启动 Flask 演示网页

示例:
  python -m price_compare.cli search "手机" --mock
  python -m price_compare.cli report "蓝牙耳机" --out output
  python -m price_compare.cli sample
  python -m price_compare.cli web --port 5000
"""
from __future__ import annotations

import argparse
import json
import logging
import os
import sys
from typing import List

from . import __version__
from .processor.pipeline import Pipeline, serialize
from .models import Product

log = logging.getLogger(__name__)

PLATFORM_CN = {"jd": "京东", "taobao": "淘宝", "pdd": "拼多多"}
TAG_CN = {"性价比推荐": "★推荐", "适中": "适中", "偏高": "偏高"}


def _setup_logging(verbose: bool):
    logging.basicConfig(
        level=logging.DEBUG if verbose else logging.INFO,
        format="%(asctime)s [%(levelname)s] %(name)s: %(message)s",
        datefmt="%H:%M:%S",
    )


def _fmt_price(v) -> str:
    return f"¥{v:,.2f}"


def _fmt_sales(n: int) -> str:
    if n >= 10000:
        return f"{n/10000:.1f}万"
    return str(n)


def _disp_w(s: str) -> int:
    """显示宽度: CJK/全角按2计, 其余按1."""
    import unicodedata
    w = 0
    for ch in s:
        w += 2 if unicodedata.east_asian_width(ch) in ("W", "F") else 1
    return w


def _pad(s: str, width: int) -> str:
    """按显示宽度右补空格对齐."""
    return s + " " * max(0, width - _disp_w(s))


def print_table(products: List[dict]) -> None:
    """以对齐表格打印商品 (价格升序)."""
    if not products:
        print("无数据")
        return
    cols = ["#", "平台", "商品名称", "价格", "销量", "店铺评分", "性价比", "店铺"]
    rows = []
    for i, p in enumerate(products, 1):
        title = p["title"]
        if len(title) > 26:
            title = title[:25] + "…"
        rows.append([
            str(i),
            PLATFORM_CN.get(p["platform"], p["platform"]),
            title,
            _fmt_price(p["price"]),
            _fmt_sales(p["sales"]),
            f"{p['shop_rating']}" if p["shop_rating"] else "-",
            TAG_CN.get(p.get("value_tag"), "-"),
            p["shop_name"][:10],
        ])
    widths = [max(_disp_w(str(c)), max(_disp_w(r[i]) for r in rows))
              for i, c in enumerate(cols)]
    sep = "+" + "+".join("-" * (w + 2) for w in widths) + "+"
    header = "|" + "|".join(f" {_pad(str(c), w)} " for c, w in zip(cols, widths)) + "|"
    print(sep); print(header); print(sep)
    for r in rows:
        print("|" + "|".join(f" {_pad(str(c), w)} " for c, w in zip(r, widths)) + "|")
    print(sep)


def print_comparison(cmp: dict) -> None:
    """打印横向对比摘要."""
    print("\n=== 横向对比摘要 ===")
    overall = cmp["overall"]
    print(f"总计 {overall['count']} 件 | 价格区间 "
          f"{_fmt_price(overall['min_price'])} ~ "
          f"{_fmt_price(overall['max_price'])} | 均价 "
          f"{_fmt_price(overall['avg_price'])} | 中位价 "
          f"{_fmt_price(overall['median_price'])}")
    print(f"总销量 {_fmt_sales(overall['total_sales'])} 件"
          f" | 平均店铺评分 {overall['avg_rating'] or '-'}")
    print("\n各平台对比:")
    hdr = ["平台", "数量", "最低价", "均价", "中位价", "均分"]
    widths = []
    rows = []
    for plat, s in sorted(cmp["per_platform"].items()):
        rows.append([PLATFORM_CN.get(plat, plat), str(s["count"]),
                     _fmt_price(s["min_price"]), _fmt_price(s["avg_price"]),
                     _fmt_price(s["median_price"]), str(s["avg_rating"] or "-")])
    for i, h in enumerate(hdr):
        widths.append(max(_disp_w(h), max(_disp_w(r[i]) for r in rows)))
    print("  " + "  ".join(_pad(h, w) for h, w in zip(hdr, widths)))
    for r in rows:
        print("  " + "  ".join(_pad(c, w) for c, w in zip(r, widths)))


def cmd_search(args) -> int:
    pipeline = Pipeline(platforms=args.platforms, mock=args.mock,
                         per_platform=args.limit)
    result = pipeline.run(args.keyword)
    print(f"\n关键词: {args.keyword} | 采集 {result['raw_count']} 件, "
          f"清洗后 {result['total']} 件\n")
    print_table(result["products"])
    print_comparison(result["comparison"])
    out_path = args.out or f"output_{args.keyword}.json"
    with open(out_path, "w", encoding="utf-8") as f:
        f.write(serialize(result))
    print(f"\n[已保存] {out_path}")
    return 0


def cmd_report(args) -> int:
    from .visualizer import Visualizer
    pipeline = Pipeline(platforms=args.platforms, mock=args.mock,
                         per_platform=args.limit)
    result = pipeline.run(args.keyword)
    print_table(result["products"])
    print_comparison(result["comparison"])
    products = [Product(**p) for p in result["products"]]
    viz = Visualizer(out_dir=args.out)
    paths = viz.render_all(products)
    print("\n=== 可视化图表 ===")
    for p in paths:
        print(f"  [图] {p}")
    with open(os.path.join(args.out, "result.json"), "w", encoding="utf-8") as f:
        f.write(serialize(result))
    print(f"\n[已保存] {os.path.join(args.out, 'result.json')}")
    return 0


def cmd_sample(args) -> int:
    """输出内置示例数据 (无需联网)."""
    sample = _load_sample()
    print("=== 示例: 采集原始数据 (片段) ===")
    raw = sample["raw_sample"]
    print_table(raw[:8])
    print(f"... 共 {len(raw)} 条原始数据\n")
    print("=== 示例: 处理结果 ===")
    print(f"关键词: {sample['result']['keyword']}")
    print_table(sample["result"]["products"])
    print_comparison(sample["result"]["comparison"])
    return 0


def cmd_web(args) -> int:
    from .web.app import create_app
    app = create_app()
    print(f"\n  电商价格采集对比工具 Web 演示启动中...")
    print(f"  访问: http://127.0.0.1:{args.port}/\n")
    app.run(host=args.host, port=args.port, debug=args.debug)
    return 0


def _load_sample() -> dict:
    """加载内置示例 (data/sample.json)."""
    here = os.path.dirname(__file__)
    path = os.path.join(here, "data", "sample.json")
    with open(path, encoding="utf-8") as f:
        return json.load(f)


def build_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(
        prog="price-compare",
        description="电商商品价格自动化采集与对比工具",
    )
    p.add_argument("--version", action="version", version=f"%(prog)s {__version__}")
    p.add_argument("-v", "--verbose", action="store_true", help="详细日志")
    sub = p.add_subparsers(dest="cmd", required=True)

    ps = sub.add_parser("search", help="关键词搜索采集并对比")
    ps.add_argument("keyword", help="搜索关键词, 如 手机/蓝牙耳机")
    ps.add_argument("--platforms", nargs="*", default=None,
                    help="平台 jd taobao pdd, 默认全部")
    ps.add_argument("--limit", type=int, default=12, help="每平台采集条数")
    ps.add_argument("--mock", action="store_true",
                    help="强制 Mock 数据 (稳定演示, 无需联网)")
    ps.add_argument("--out", default=None, help="结果 JSON 输出路径")
    ps.set_defaults(func=cmd_search)

    pr = sub.add_parser("report", help="采集+对比+可视化图表")
    pr.add_argument("keyword", help="搜索关键词")
    pr.add_argument("--platforms", nargs="*", default=None)
    pr.add_argument("--limit", type=int, default=12)
    pr.add_argument("--mock", action="store_true")
    pr.add_argument("--out", default="output", help="输出目录")
    pr.set_defaults(func=cmd_report)

    psamp = sub.add_parser("sample", help="输出内置示例数据与结果")
    psamp.set_defaults(func=cmd_sample)

    pw = sub.add_parser("web", help="启动 Flask 演示网页")
    pw.add_argument("--host", default="127.0.0.1")
    pw.add_argument("--port", type=int, default=5000)
    pw.add_argument("--debug", action="store_true")
    pw.set_defaults(func=cmd_web)
    return p


def main(argv: List[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    _setup_logging(args.verbose)
    try:
        return args.func(args)
    except KeyboardInterrupt:
        print("\n已中断")
        return 130


if __name__ == "__main__":
    sys.exit(main())
