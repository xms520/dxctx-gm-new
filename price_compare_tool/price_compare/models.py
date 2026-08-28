"""数据模型: 商品 (Product) 与采集/分析结果."""
from __future__ import annotations

import time
from dataclasses import dataclass, field, asdict
from typing import Optional


@dataclass
class Product:
    """单个商品的标准化数据结构.

    统一封装各平台采集结果, 贯穿采集 -> 清洗 -> 对比 -> 分析全流程.
    """

    platform: str            # jd / taobao / pdd
    keyword: str             # 采集时使用的搜索关键词
    title: str               # 商品名称
    price: float             # 标准化价格 (元)
    sales: int               # 标准化销量 (件)
    shop_name: str           # 店铺名称
    shop_rating: Optional[float] = None  # 店铺评分 (4.x ~ 5.0)
    url: str = ""            # 商品链接
    image_url: str = ""      # 主图链接 (可选)
    value_score: Optional[float] = None     # 性价比评分 (分析阶段填充)
    value_tag: Optional[str] = None         # 性价比标签: 推荐/适中/偏高
    rank_in_platform: Optional[int] = None  # 平台内价格排名
    collected_at: int = field(default_factory=lambda: int(time.time()))

    def to_dict(self) -> dict:
        return asdict(self)

    def dedup_key(self) -> tuple:
        """用于跨平台去重的业务键 (标题归一化 + 价格)."""
        import re
        norm_title = re.sub(r"\s+", "", self.title).lower()[:40]
        return (self.platform, norm_title, round(self.price, 2))
