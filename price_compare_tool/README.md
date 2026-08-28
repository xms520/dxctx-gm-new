# 电商商品价格自动化采集与对比工具

从京东 / 淘宝 / 拼多多 **批量采集** 商品名称、价格、销量、店铺评分、链接等信息，
自动 **清洗去重 → 价格升序排序 → 横向对比 → 性价比推荐标注 → 可视化**，
提供 **命令行工具** 与 **演示网页**（网页支持输入关键词现场运行，初始化即给出数据示例与结果示例）。

---

## 一、整体架构设计

```
┌─────────────────────────────────────────────────────────────┐
│                         用户入口                              │
│   CLI (cli.py)            Web 演示页 (web/app.py + index.html) │
└───────────────┬───────────────────────────┬───────────────────┘
                │                           │
                ▼                           ▼
┌─────────────────────────────────────────────────────────────┐
│              Pipeline 流水线编排 (processor/pipeline.py)        │
│   关键词 → 采集 → 清洗 → 分析 → 对比 → 输出统一结果结构          │
└───────┬─────────────┬───────────────┬───────────────┬───────┘
        ▼             ▼               ▼               ▼
 ┌──────────┐  ┌────────────┐  ┌────────────┐  ┌────────────┐
 │ 采集层    │  │ 清洗层     │  │ 分析层     │  │ 对比层     │
 │ scrapers │  │ Cleaner   │  │ Analyzer   │  │ Comparator│
 │ /manager │  │ 去重/归一化 │  │ 性价比打分 │  │ 聚合统计   │
 └────┬─────┘  └────────────┘  └────────────┘  └────────────┘
      │
      ▼
 ┌───────────────────────────────────────────────┐
 │  ScraperManager (并发调度多平台)                 │
 │  ┌──────┐  ┌────────┐  ┌──────┐               │
 │  │ JD   │  │ Taobao │  │ PDD  │  各自注入       │
 │  │真实+ │  │真实+   │  │真实+ │  MockScraper     │
 │  │Mock兜│  │Mock兜  │  │Mock兜│  兜底            │
 │  └──────┘  └────────┘  └──────┘               │
 └───────────────────────────────────────────────┘
```

**分层职责：**

| 层 | 模块 | 职责 |
|----|------|------|
| 采集层 | `scrapers/` | 各平台爬虫 + Mock 兜底，统一产出 `Product` |
| 处理层 | `processor/` | 清洗去重、性价比打分、横向对比统计 |
| 可视化 | `visualizer/` | matplotlib 静态图表（CLI）；Web 用 ECharts 动态渲染 |
| 入口层 | `cli.py` / `web/` | 命令行与 Web 演示，共用同一 `Pipeline` |

**核心数据结构** `Product`（`models.py`）：`platform / keyword / title / price / sales / shop_name / shop_rating / url / image_url / value_score / value_tag / rank_in_platform / collected_at`，贯穿全流程，保证采集与下游无耦合。

---

## 二、数据获取核心逻辑

各平台反爬差异极大，采用 **“真实采集优先，Mock 兜底”** 策略（`scrapers/base.py`）：

1. **真实采集优先**：先调用平台 `_search_real`。
2. **失败兜底**：网络/反爬/鉴权失败或返回空时，自动回退到该平台的 `MockScraper`，生成结构相同的演示数据，保证工具**任何环境可运行可演示**。
3. **同构输出**：真实与 Mock 产出完全相同的 `Product`，下游清洗/对比/分析无感知。

| 平台 | 真实路径 | 现状 |
|------|----------|------|
| 京东 | `search.jd.com` 商品搜索 HTML 页（未登录可访问），BeautifulSoup 解析 `li.gl-item` | 可直接解析；失败兜底 Mock |
| 淘宝 | `s.taobao.com` 需登录 Cookie/签名 | 无 Cookie 命中登录墙→Mock 兜底；注入 `TB_COOKIE` 可启用真实 |
| 拼多多 | `mobile.yangkeduo.com` 需 app 签名+token | 无 Cookie→Mock 兜底；注入 `PDD_COOKIE` 可启用真实 |

> 启用真实采集：`export TB_COOKIE="..."` 后运行（不勾选 Mock）。沙箱/无凭据环境请使用 `--mock` 获得稳定演示。

**并发**：`ScraperManager` 用 `ThreadPoolExecutor` 并发调度多平台，互不阻塞。

---

## 三、数据处理与可视化方案

### 清洗 (`processor/cleaner.py`)
- 价格归一（`¥99.9` / `1万+评价` → 数值）
- 销量归一（`1.2万` → 12000）
- 标题空白规整、缺失评分补 `None`
- 去重：业务键 `(平台, 标题归一前40字符, 价格)`
- 排序：默认价格升序（可按性价比/销量）

### 性价比分析 (`processor/analyzer.py`)
```
value_score = 0.5*(1−价格归一)  +  0.3*销量归一  +  0.2*店铺评分归一   ∈ [0,1]
```
- Top 20% → `性价比推荐`（绿）；中 55% → `适中`（黄）；后 25% → `偏高`（红）

### 横向对比 (`processor/comparator.py`)
按平台 + 整体聚合：数量 / 最低 / 最高 / 均价 / 中位价 / 总销量 / 平均评分，并计算**平台内价格排名**。

### 可视化
| 场景 | 方案 |
|------|------|
| CLI | `visualizer/charts.py` matplotlib(Agg) 输出 PNG：各平台均价柱状、价格-销量散点、性价比 Top10 条形 |
| Web | ECharts 动态渲染：各平台均价/中位价、价格-销量散点、性价比 Top10、价格阶梯分布 |

> “价格趋势”：本工具为实时快照（无历史数据库）。Web 端以 **价格阶梯分布**（按价格升序）呈现同关键词下商品价格跨度与各平台定位，CLI 同理。如需历史趋势，可周期性运行并将结果存库后增量绘图。

---

## 四、安装与运行

```bash
cd price_compare_tool
pip install -r requirements.txt
```

### CLI 命令行工具

```bash
# 1) 查看内置示例 (数据示例 + 结果示例, 无需联网)
python -m price_compare.cli sample

# 2) 关键词搜索采集 (Mock 演示)
python -m price_compare.cli search "手机" --mock

# 3) 真实采集 (京东 HTML 解析; 淘宝/拼多多需注入 Cookie)
python -m price_compare.cli search "蓝牙耳机"
export TB_COOKIE="..."; export PDD_COOKIE="..."
python -m price_compare.cli search "蓝牙耳机" --platforms jd taobao pdd

# 4) 完整报告 (采集+对比+图表 PNG)
python -m price_compare.cli report "机械键盘" --mock --out output

# 5) 启动 Web 演示页
python -m price_compare.cli web --port 5000
```

CLI 子命令：`search` / `report` / `sample` / `web`，参数见 `python -m price_compare.cli --help`。

### Web 演示页

```bash
python -m price_compare.cli web --port 5000
# 访问 http://127.0.0.1:5000/
```

- **初始化**：首页预载内置 `sample.json` —— 同时展示**采集原始数据示例**与**处理结果示例**（含表格、对比摘要、4 张图表）。
- **现场运行**：输入关键词 → 选择平台 → 勾选 Mock → 点“运行采集”，调用 `/api/search` 现场执行 Pipeline，结果表格与图表实时刷新。
- **接口**：`GET /api/sample`、`GET|POST /api/search?keyword=...&mock=1`。

---

## 五、项目结构

```
price_compare_tool/
├── requirements.txt
├── _gen_sample.py                 # 重新生成内置示例 data/sample.json 的工具脚本
└── price_compare/
    ├── __init__.py
    ├── models.py                  # Product 数据模型
    ├── cli.py                     # CLI 入口 (search/report/sample/web)
    ├── scrapers/
    │   ├── base.py                # 采集基类: 真实优先 + Mock 兜底
    │   ├── jd.py                  # 京东 (search.jd.com HTML 解析)
    │   ├── taobao.py              # 淘宝 (Cookie 真实 + Mock 兜底)
    │   ├── pdd.py                 # 拼多多 (Cookie 真实 + Mock 兜底)
    │   ├── mock.py                # 确定性 Mock 数据生成
    │   └── manager.py            # 多平台并发调度
    ├── processor/
    │   ├── cleaner.py             # 清洗/去重/排序
    │   ├── comparator.py          # 横向对比统计
    │   ├── analyzer.py            # 性价比打分 + 推荐标注
    │   └── pipeline.py           # 端到端流水线 (CLI/Web 共用)
    ├── visualizer/
    │   └── charts.py              # matplotlib 图表 (CLI)
    ├── web/
    │   ├── app.py                 # Flask 路由
    │   └── templates/index.html   # 演示页 (ECharts)
    └── data/
        └── sample.json            # 内置示例数据 + 结果
```

---

## 六、重新生成示例数据

修改 Mock 逻辑后可重新生成内置示例（保证首页示例与流水线一致）：

```bash
python _gen_sample.py        # → price_compare/data/sample.json
```
