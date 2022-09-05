# 空戦AIチャレンジ シミュレーターソースコード

## 環境構築手法

### 事前準備
[Nishikaコンペページ](https://www.nishika.com/competitions/23/data)からダウンロード可能な以下データを準備する。
- simulator.zip
- Dockerfile
- py37_linux_whl.zip / py37_mac_whl.zip / py37_win_msvc_whl.zip / py37_win_msys_whl.zip

### 手法1 Ubuntuベースのコンテナ環境を構築
simulator.zipを解凍する。

解凍したsimulatorのrootにDockerfileを置き、以下を実行する。
```
docker build .
```
この環境が提出されたエージェント同士の対戦環境にもなっている。

コンテナ環境の中に入り、以下動作確認用サンプルを実行し問題なければ成功である。
```
cd sample/Standard
python FirstSample.py
```
上記コンテナ環境はGUI非対応であるため、後に示す、戦闘画面の可視化を行いたい場合は手法2を推奨する。

### 手法2-1 ホストOS上で直接環境構築（Linux, macOS） ※推奨

Linuxの場合はpy37_linux_whl.zip、macOSの場合はpy37_mac_whl.zipを解凍し、2つのwhlファイルが存在することを確認する。<br>
Python3.7がインストールされた仮想環境等にて、以下を実行する。
```
pip install "ray[default, tune, rllib]==1.9.1"
pip install ASRCAISim1-1.0.0-py3-none-any.whl
pip install OriginalModelSample-1.0.0-py3-none-any.whl
```

nloptをインストールする。
```
curl -sSL https://github.com/stevengj/nlopt/archive/v2.6.2.tar.gz | tar xz
cd nlopt-2.6.2 && cmake . && make && make install
```

nloptの共有オブジェクトの場所を指定する。
Linuxの場合は、
```
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib
```
macOSの場合は、
```
export LIBRARY_PATH=$LIBRARY_PATH:/usr/local/lib
```
により指定できる。

simulator.zipを解凍し、以下動作確認用サンプルを実行し問題なければ成功である。
```
cd sample/Standard
python FirstSample.py
```

### 手法2-2 ホストOS上で直接環境構築（Windows, MSVC）

py37_win_msvc_whl.zipを解凍し、2つのwhlファイルが存在することを確認する。<br>
Python3.7がインストールされた仮想環境等にて、以下を実行する。
```
pip install "ray[default, tune, rllib]==1.9.1"
pip install ASRCAISim1-1.0.0-py3-windows-any.whl
pip install OriginalModelSample-1.0.0-py3-windows-any.whl
```

nloptをインストールする。
```
mkdir c:\simulator;cd simulator
curl -sSL -o nlopt-2.6.2.tar.gz https://github.com/stevengj/nlopt/archive/v2.6.2.tar.gz
tar -xzf nlopt-2.6.2.tar.gz
cd nlopt-2.6.2
mkdir build;cd build
cmake ..
```
Visual Studioを起動して、nlopt.slnを開いて Releaseモードでビルドする。

システム環境変数Pathに、以下のnlopt.dllへのパスとASRCAISim1のCore.dllへのパス（自身のパスに読み替えること）を追加する。
```
C:\simulator\nlopt-2.6.2\build\Release
C:\Users\...\lib\site-packages\ASRCAISim1
```

simulator.zipを解凍し、以下動作確認用サンプルを実行し問題なければ成功である。
```
cd sample/Standard
python FirstSample.py
```

### 手法2-3 ホストOS上で直接環境構築（Windows, MSYS）

py37_win_msys_whl.zipを解凍し、2つのwhlファイルが存在することを確認する。<br>
Python3.7がインストールされた仮想環境等にて、以下を実行する。
```
pip install "ray[default, tune, rllib]==1.9.1"
pip install ASRCAISim1-1.0.0-py3-windows-any.whl
pip install OriginalModelSample-1.0.0-py3-windows-any.whl
```

nloptをインストールする。
```
mkdir c:\simulator;cd simulator
curl -sSL -o nlopt-2.6.2.tar.gz https://github.com/stevengj/nlopt/archive/v2.6.2.tar.gz
tar -xzf nlopt-2.6.2.tar.gz
cd nlopt-2.6.2
mkdir build;cd build
cmake .. -G "MSYS Makefiles"
make; make install
```

システム環境変数Pathに、以下のnlopt.dllへのパスとASRCAISim1のCore.dllへのパス（自身のパスに読み替えること）を追加する。
```
C:\simulator\nlopt-2.6.2\build\Release
C:\Users\...\lib\site-packages\ASRCAISim1
```

simulator.zipを解凍し、以下動作確認用サンプルを実行し問題なければ成功である。
```
cd sample/Standard
python FirstSample.py
```

## 動作確認用サンプル
root/sample/Standard配下の、動作確認用サンプルについて説明する。

### シミュレータとしての動作確認用サンプル
FirstSample.pyは、ルールベースの行動判断モデル同士の対戦が実行される。
```
cd sample/Standard
python FirstSample.py
```
で実行される。<br>
ObservationやActionの入出力は伴わない。

### OpenAI gym環境としてのI/F確認用サンプル
SecondSample.pyでは、サンプルのAgentモデル２種（Blue側は1機ずつ行動、Red側は２機分一纏めに行動）同士のランダム行動による対戦が実行される。
```
cd sample/Standard
python SecondSample.py
```
で実行される。<br>
ObservationやActionの定義例を確認できる。


## 任意の陣営間の対戦の実行（陣営ごとにAgentやPolicyを隔離した状態で対戦）

陣営ごとにAgentやPolicyの動作環境や設定ファイルをパッケージ化し、互いに隔離した状態で対戦させて評価を行うことを可能にするための機能を
root/sample/MinimumEvaluation及びroot/addons/AgentIsolationに実装している。

### Agent及びPolicyのパッケージ化の方法

AgentとPolicyの組を一意に識別可能な名称をディレクトリ名とし、`__init__.py`を格納してPythonモジュールとしてインポート可能な状態とし、
インポートにより以下の4種類の関数がロードされるような形で実装されていれば、細部の実装方法は問わないものとする。

(1) getUserAgentClass()･･･Agentクラスオブジェクトを返す関数<br>
(2) getUserAgentModelConfig()･･･AgentモデルのFactoryへの登録用にmodelConfigを表すjson(dict)を返す関数<br>
(3) isUserAgentSingleAsset()･･･Agentの種類(一つのAgentインスタンスで1機を操作するのか、陣営全体を操作するのか)をboolで返す関数(Trueが前者)<br>
(4) getUserPolicy()･･･StandalonePolicyを返す関数

### パッケージ化されたAgent及びPolicyの組を読み込んで対戦させるサンプル

陣営ごとにAgent及びPolicyを隔離して対戦させる場合、<br>
まず、sep_config.jsonにBlueとRedのそれぞれに対応するedgeのIPアドレス、ポート名、Agent・Policyパッケージの識別名を記述しておく。設定例は以下。

```json
{
    "blue":{
        "userID":"RuleBased",
        "server":"localhost",
        "agentPort":51000,
        "policyPort":51001
    },
    "red":{
        "userID":"User001",
        "server":"localhost",
        "agentPort":51002,
        "policyPort":51003
    },
    "seed":null
}
```

次に、Blueを動かすedge環境において
```
python sep_edge.py blue
```
Redを動かすedge環境において
```
python sep_edge.py red
```
をそれぞれ実行した後に、center環境において
```
python sep_main.py
```
を実行することで対戦させることが可能。


## 戦闘画面の可視化方法・対戦ログの出力方法

root/sample/MinimumEvaluation配下のサンプルでは、sep_main.pyのconfigを"ViewerType": "God"と書き換えることで戦闘画面が可視化される。
```python
    configs = [
        os.path.join(os.path.dirname(__file__), "common/BVR2v2_rand.json"),
        agentConfig,
        {
            "Manager": {
                "Rewards": [],
                "seed":seed,
                "ViewerType":"God",
                "Loggers":{
                }
            }
        }
    ]
```

root/sample/MinimumEvaluation配下のサンプルでは、sep_main.pyのconfigの"Loggers"を以下の通り書き換えることでログが出力される。
```python
                "Loggers":{
                    "GodViewStateLogger":{
                        "class":"GodViewStateLogger",
                        "config":{
                            "prefix":"./results/GodViewStateLog",
                            "episodeInterval":1,
                            "innerInterval":1
                        }
                    }
                }
```

出力したログを元に戦闘画面を再度可視化するには、<br>
root/sample/MinimumEvaluation/replay.pyの第一引数としてログの.datファイルを、第二引数として動画や連番画像を保存するファイルパス名prefix（以下の例の場合./movies/movie_e0001.mp4として保存される）を指定して実行することで、可視化される。
```
python replay.py "./results/GodViewStateLog_YYYYMMDDhhrrss_e0001.dat" "./movies/movie"
```


## [参考] その他の動作確認用サンプル

### ray RLlibを用いた学習のサンプル

#### 模倣学習用の教師データの取得
```
cd sample/Standard
python ExpertTrajectoryGatherer.py Gather2v2.json
```
を実行すると、./experts/2vs2/以下にルールベースの行動に相当する教師データが保存される。

#### 模倣学習の実施
教師データの取得後、同フォルダで
```
python ImitationSample.py Imitate2v2.json
```
を実行すると、./experts/2vs2/以下の教師データを入力として模倣学習が行われ、
完了時には./policies/Imitated2v2.datに重みが保存される。
また、実行中のチェックポイントは./results/Imitate2v2/run_YYYY-mm-dd-HH-MM-SS/以下に保存される。

#### 強化学習の実施
模倣学習の実施後、同フォルダで
```
python LearningSample.py IMPALA2v2.json
```
を実行すると、./policies/Imitated2v2.datを初期重みとしてIMPALAによる強化学習が行われ、
完了時には./policies/IMPALA2v2.datに重みが保存される。
また、実行中のチェックポイントは./results/IMPALA2v2/run_YYYY-mm-dd-HH-MM-SS/以下に保存される。
なお、初期重みを使用しない場合はjson中のパスを指定している部分をnullとすればよい。

#### 学習済モデルの評価
模倣学習の実施後は同フォルダで
```
python ImitatedTest.py Imitate2v2.json ./policies/Imitated2v2.dat
```
強化学習の実施後は同フォルダで
```
python LearnedTest.py IMPALA2v2.json ./policies/IMPALA2v2.dat
```
を実行すると、学習済モデルを読み込みルールベースモデルと対戦させ、その結果を記録することができる。

#### サンプルのバリエーション
上記のサンプルは、1つのエージェントで1機を動かすモデルを学習するものとなっているが、
sample/Standardフォルダにはこれ以外に以下の2種類のサンプルが含まれており、上記実行例の読み込むjsonファイルを変更することで同様に使用可能である。

##### 1つのエージェントで2機両方を動かすモデルを学習するサンプル
Gather2v2_Cent.json, Imitate2v2_Cent.json, IMPALA2v2_Cent.jsonにそれぞれ置き換えればよい。
ただし、学習済モデルの評価は、ImitatedTest.py及びLearnedTest.py中のmain関数において
```python
configs=["../config/BVR2v2_rand.json","../config/Learned2v2.json"]
```
となっている部分を
```python
configs=["../config/BVR2v2_rand.json","../config/Learned2v2_Cent.json"]
```
とする必要がある。

##### 機体モデルを更に簡略化した質点モデルを用いた環境で学習するサンプル
Gather2v2_MP.json, Imitate2v2_MP.json, IMPALA2v2_MP.jsonにそれぞれ置き換えればよい。
ただし、学習済モデルの評価は、ImitatedTest.py及びLearnedTest.py中のmain関数において
```python
configs=["../config/BVR2v2_rand.json","../config/Learned2v2.json"]
```
となっている部分を
```python
configs=["../config/BVR2v2_rand_MP.json","../config/Learned2v2.json","./config_MP.json"]
```
とする必要がある。# simulator-2
