"""Mock 数据生成器.

用途:
  1. 作为真实采集失败时的兜底, 保证工具在任何环境可运行/可演示;
  2. 为 Web 演示页提供初始化示例数据.

特性:
  - 基于关键词 hash 生成确定性数据 (同关键词 -> 同数据, 便于复现演示);
  - 各平台价格分布符合真实直觉: 拼多多最低、京东中高、淘宝跨度最大;
  - 商品标题/店铺/链接各平台风格一致.
"""
from __future__ import annotations

import hashlib
import random
from typing import List

from ..models import Product
from .base import BaseScraper

# 平台 -> 价格区间与 URL 模板
_PLATFORM_PROFILE = {
    "jd": {
        "price": (129.0, 4999.0),      # 京东偏中高
        "shops": ["京东自营", "小米官方旗舰店", "华为官方旗舰店",
                  "苹果京东自营", "联想京东自营", "索尼旗舰店"],
        "rating": (4.7, 5.0),
        "url_tpl": "https://item.jd.com/{sku}.html",
        "img_tpl": "https://img{img}.360buyimg.com/n1/jfs/t{sku}/{sku}.jpg",
    },
    "taobao": {
        "price": (39.0, 3999.0),       # 淘宝跨度大, 低端多
        "shops": ["天猫超市", "小米天猫旗舰店", "荣耀官方旗舰店",
                  "OPPO官方旗舰店", "vivo官方旗舰店", "数码专营店"],
        "rating": (4.6, 4.99),
        "url_tpl": "https://detail.tmall.com/item.htm?id={sku}",
        "img_tpl": "https://img.alicdn.com/imgextra/i{sku}/{sku}.jpg",
    },
    "pdd": {
        "price": (9.9, 1999.0),        # 拼多多最低
        "shops": ["百亿补贴官方", "小米拼多多旗舰店", "数码优品馆",
                  "工厂直营店", "品牌特卖", "拼购优选"],
        "rating": (4.5, 4.95),
        "url_tpl": "https://mobile.yangkeduo.com/goods.html?goods_id={sku}",
        "img_tpl": "https://img.pddpic.com/goods/{sku}.jpg",
    },
}

# 用于拼装商品标题的修饰词
_PREFIX = ["官方正品", "新品", "热销款", "旗舰版", "国行", "现货", "百亿补贴"]
_SUFFIX = ["包邮", "次日达", "全国联保", "七天无理由", "假一赔十", "送运费险"]


class MockScraper(BaseScraper):
    """按平台生成模拟商品数据."""

    allow_mock_fallback = False  # 兜底器本身不需要再兜底, 避免递归

    def __init__(self, platform: str):
        super().__init__(mock_provider=None)
        self.platform = platform
        if platform not in _PLATFORM_PROFILE:
            raise ValueError(f"未知平台: {platform}")
        self._profile = _PLATFORM_PROFILE[platform]

    def search(self, keyword: str, limit: int = 20) -> List[Product]:
        # 覆写 search 直接生成, 跳过 _search_real 兜底逻辑
        return self._generate(keyword, limit)

    def _search_real(self, keyword: str, limit: int) -> List[Product]:
        # 不会被调用 (search 已覆写), 仅为满足抽象基类
        return self._generate(keyword, limit)

    def _generate(self, keyword: str, limit: int) -> List[Product]:
        """基于关键词 hash 生成确定性数据."""
        seed = int(hashlib.md5((self.platform + "|" + keyword).encode()).hexdigest(), 16)
        rng = random.Random(seed)
        n = min(limit, 12)
        items: List[Product] = []
        for i in range(n):
            kw_brand = self._brand_for(keyword, rng)
            spec = self._spec_for(keyword, rng)
            model = f"{rng.randint(100, 999)}系列"
            title = f"{self._pick(rng, _PREFIX)} {kw_brand} {keyword} {model} {spec} {self._pick(rng, _SUFFIX)}"

            price = round(self._price(rng), 2)
            sales = self._sales(rng)
            shop = self._pick(rng, self._profile["shops"])
            rating = round(rng.uniform(*self._profile["rating"]), 2)

            sku = rng.randint(10_000_000, 99_999_999)
            url = self._profile["url_tpl"].format(sku=sku)
            img = self._profile["img_tpl"].format(sku=sku, img=sku % 100)

            items.append(Product(
                platform=self.platform,
                keyword=keyword,
                title=self._tidy(title),
                price=price,
                sales=sales,
                shop_name=shop,
                shop_rating=rating,
                url=url,
                image_url=img,
            ))
        return items

    # ---- 辅助生成函数 ----
    def _price(self, rng: random.Random) -> float:
        lo, hi = self._profile["price"]
        # 用对数分布模拟长尾: 大部分低价, 少量高价
        import math
        r = rng.random() ** 1.7
        return lo + (hi - lo) * r

    def _sales(self, rng: random.Random) -> int:
        # 销量长尾分布: 几十到几十万
        base = rng.choices(
            [rng.randint(10, 999), rng.randint(1000, 9999),
             rng.randint(10_000, 99_999), rng.randint(100_000, 500_000)],
            weights=[5, 4, 2, 1], k=1
        )[0]
        return base

    def _brand_for(self, keyword: str, rng: random.Random) -> str:
        brands = ["小米", "华为", "苹果", "联想", "索尼", "三星", "OPPO",
                  "vivo", "荣耀", "realme", "一加", "戴尔", "惠普"]
        return rng.choice(brands)

    def _spec_for(self, keyword: str, rng: random.Random) -> str:
        specs = ["8GB+256GB", "12GB+512GB", "16GB+1TB", "256GB", "512GB",
                 "Pro Max", "Ultra", "标准版", "顶配版", "5G版"]
        return rng.choice(specs)

    def _pick(self, rng: random.Random, pool: List[str]) -> str:
        return rng.choice(pool)

    def _tidy(self, title: str) -> str:
        return " ".join(title.split())
