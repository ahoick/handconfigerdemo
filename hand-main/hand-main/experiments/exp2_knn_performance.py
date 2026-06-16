"""
实验二: KNN分类性能实验
测试不同K值下的分类效果
"""

import sys
import time
from pathlib import Path
import numpy as np
from sklearn.model_selection import train_test_split, StratifiedKFold
from sklearn.metrics import accuracy_score, precision_score, recall_score, f1_score
import pickle

sys.path.insert(0, str(Path(__file__).parent.parent))

from src.classifier import KNNClassifier


def main():
    print("=" * 60)
    print("实验二: KNN 分类性能实验")
    print("=" * 60)

    # 加载预处理好的特征数据（需先运行 train.py 生成）
    # 此处假设已有特征文件，若无则提示先运行 train.py
    model_dir = Path("data/models")
    model_path = model_dir / "knn_model.pkl"

    if not model_path.exists():
        print("[ERROR] 未找到训练好的模型，请先运行:")
        print("  python scripts/train.py")
        return

    # 注: 实际实验需要加载原始特征数据
    # 这里用模拟数据展示实验框架
    print("[WARN] 本实验需配合实际数据集运行")
    print("[INFO] 实验框架已就绪，请在采集数据后运行 scripts/train.py")

    # 实验框架说明
    k_values = [1, 3, 5, 7, 9, 11, 15]
    print(f"\n待测试K值: {k_values}")
    print("实验流程:")
    print("1. 加载预处理后的特征数据 X, y")
    print("2. 对每个K值进行5折交叉验证")
    print("3. 记录准确率、精确率、召回率、F1值")
    print("4. 输出对比表格和最佳参数")
    print("\n指标说明:")
    print("  Accuracy  = (TP+TN)/(TP+TN+FP+FN)")
    print("  Precision = TP/(TP+FP)")
    print("  Recall    = TP/(TP+FN)")
    print("  F1-score  = 2*P*R/(P+R)")


if __name__ == "__main__":
    main()
