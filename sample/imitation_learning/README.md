## 模倣学習
本章では模倣学習と呼ばれる方法でモデルを構築する方法を紹介する。  
サンプルにエキスパートが設定したルールを持つエージェントが存在しており、それをモデルに学習させることで同様の動作を獲得できる。

### 模倣学習用のデータの生成
「sample/Standard」以下のプログラムを実行する。
```
python ExpertTrajectoryGatherer.py　Gather2v2.json
```

実行結果として、以下が得られる。生成されたエピソードは次の箇所に出力される。
sample/Standard/result/Traj*/

### 模倣学習
模倣学習用により、モデルを作成するプログラムは以下に配置している。
次のプログラムを実行することで模倣学習を実行できる。
下記のフォルダパスはこのサンプルであれば、「sample/Standard/result/Traj*」を入力とする。  

```
python imitation_learning.py -i <フォルダのパス>
```

学習結果は、同階層の「imitation_model.pth」に出力される。

### 学習結果による確認
「MinimumEvaluation/User005」配下にディレクトリ構成があるので、モデルを作成したファイルで置き換える。  