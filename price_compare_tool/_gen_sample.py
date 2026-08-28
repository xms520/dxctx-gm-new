"""一次性脚本: 生成内置示例 data/sample.json (含原始采集示例 + 处理结果示例)."""
import json
import os

from price_compare.scrapers.manager import ScraperManager
from price_compare.processor.cleaner import Cleaner
from price_compare.processor.analyzer import Analyzer
from price_compare.processor.comparator import Comparator
from price_compare.processor.cleaner import sort_by_price

KW = "手机"
OUT = os.path.join(os.path.dirname(__file__), "price_compare", "data", "sample.json")

mgr = ScraperManager(mock=True, per_platform=10)
raw = mgr.search(KW)
raw_sample = [p.to_dict() for p in raw[:12]]  # 原始采集示例片段

cleaner = Cleaner()
analyzer = Analyzer()
comparator = Comparator()
cleaned = cleaner.run(list(raw))
ranked = analyzer.run(cleaned)
cmp = comparator.run(ranked)
final = sort_by_price(ranked, ascending=True)

result = {
    "keyword": KW,
    "total": len(final),
    "raw_count": len(raw),
    "products": [p.to_dict() for p in final],
    "comparison": cmp,
}
sample = {"raw_sample": raw_sample, "result": result}
with open(OUT, "w", encoding="utf-8") as f:
    json.dump(sample, f, ensure_ascii=False, indent=2)
print(f"已生成示例: {OUT}")
print(f"原始 {len(raw_sample)} 条, 处理后 {len(final)} 条")
