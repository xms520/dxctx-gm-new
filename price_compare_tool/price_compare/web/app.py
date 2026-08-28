"""Flask Web 演示应用.

路由:
  GET  /               渲染演示页 (预载内置示例数据 + 结果示例)
  GET  /api/sample      返回内置示例 JSON
  POST /api/search      现场运行采集流水线 (keyword) -> 结果 JSON
  GET  /api/search       同上 (GET 形式, 便于调试)

初始化时首页预载示例数据, 满足 "初始化给数据示例和结果示例" 要求.
"""
from __future__ import annotations

import json
import logging
import os
from typing import Tuple

from flask import Flask, jsonify, render_template, request

from ..processor.pipeline import Pipeline

log = logging.getLogger(__name__)

_SAMPLE_PATH = os.path.join(os.path.dirname(__file__), "..", "data",
                            "sample.json")


def _load_sample() -> dict:
    try:
        with open(_SAMPLE_PATH, encoding="utf-8") as f:
            return json.load(f)
    except FileNotFoundError:
        return {"raw_sample": [], "result": {"keyword": "", "total": 0,
                "products": [], "comparison": {"per_platform": {},
                "overall": {}, "ranked_products": []}}}


def create_app() -> Flask:
    app = Flask(__name__, static_folder="static", template_folder="templates")
    app.config["JSON_AS_ASCII"] = False

    @app.route("/")
    def index():
        sample = _load_sample()
        return render_template("index.html", sample=json.dumps(sample, ensure_ascii=False))

    @app.route("/api/sample")
    def api_sample():
        return jsonify(_load_sample())

    @app.route("/api/search", methods=["GET", "POST"])
    def api_search() -> Tuple:
        keyword = (request.form.get("keyword") or request.args.get("keyword")
                   or "").strip()
        if not keyword:
            return jsonify({"error": "缺少 keyword 参数"}), 400
        mock = (request.form.get("mock", "1") in ("1", "true", "on")
                or request.args.get("mock", "1") in ("1", "true", "on"))
        try:
            pipeline = Pipeline(mock=mock)
            result = pipeline.run(keyword)
            return jsonify(result)
        except Exception as e:  # noqa: BLE001
            log.exception("[web] search 失败")
            return jsonify({"error": str(e), "keyword": keyword}), 500

    @app.route("/health")
    def health():
        return jsonify({"status": "ok"})

    return app


app = create_app()
