### allbuild.sh
```bash
./allbuild.sh <tm>
```
- 指定のバックエンド(norec, tinystm, swisstm, tl2)で全プログラムをビルドする．
- 実行ファイルは`./build/<tm>`に入る

### build.sh
```bash
./build.sh <tm> <bench> [-b | --build]
```
- 指定のバックエンドで指定のプログラムをビルドする．
- -b(--build) オプションを付けるとバックエンドのビルドも行う
  - `(cd <tm> && make)`のこと

## 構成
> 重要と思われるディレクトリやファイルについてまとめておきます．

```
tm-study-pact14
├─ backends/      ... 各バックエンド（TMとかロックとか）でビルドするためのヘッダファイルやMakefile
├─ common/        ... 共通のMakefile群
│   └─ env.mk    ... メモリアロケータとかスレッド実装を切り替えるためのMakefile
├─ lib/           ... ライブラリ群．データ構造類は大抵ここに入っている
├─ tinySTM-1.0.5/ ... TinySTM本体
├─ ULT/
│   ├─ libfibre/ ... libfibre本体
│   └─ fibre/    ... pthreadをラップするためのファイル群
├─ inputs/        ... 入力ファイル（kmeans, labyrinth, yada）
└─ *.sh           ... スクリプト（後ろで説明）
```
TinySTMやlibfibreは，最初や修正を加えた際にビルドしてください．<br>
TinySTMなら```(cd tinySTM-1.0.5 && make clean && make)```で，
libfibreなら```(cd ULT/libfibre && make clean && make)```でビルドできるはずです．

## スクリプト
> スクリプトの使い方を示しておきますが，自分が使いやすいように適宜書き換えてください．

### sbuilder.sh
```
$ ./sbuilder.sh <backend> <program> [<out_dir>]
```
* 例) `./sbuilder.sh tinystm intruder`
* `env.mk` でスレッド実装やメモリアロケータの設定をします。
* 実行ファイルは，`build/<backend>` に置かれます。
    - `<out_dir>` を指定した場合は，`build/<out_dir>` になります。
* `-b` または `--build` オプションをつけると，`<backend>` のビルドも行います。
    - ビルドが必要な `<backend>` は STM だけです（現状は TinySTM のみ）。
    - `<backend>` がビルドしてあれば，このオプションをつける必要はありません。
        + TinySTM などを書き換えたが，まだビルドしていないという時に使います。
* `-d` オプションをつけると，その後ろに指定したソースディレクトリでビルドを行います。
    - `<program>` と ビルドしたいソースコードがあるディレクトリが異なる場合に使います。
    - 例) `./sbuilder.sh tinystm genome -d temp`
        + genome コードが `temp` というディレクトリにある場合にビルドする例です。

### run.sh
```
$ ./run.sh <build> <program> <num_of_core> [<thr_per_core>]
```
* `<build>` に指定するものは，`build/` 内のディレクトリ名です。
    - `sbuilder.sh` で指定した `<backend>` もしくは `<out_dir>` になると思います。
* `<thr_per_core>` は，1コアあたりのスレッド数を指定します。
    - 1コアあたりのスレッド数は，`env.mk` でコンパイル時に指定するようにしていますが，
      環境変数を用いて動的に変更できるようにもなっています。
* `-g` オプションをつけると，デバッガ（gdb）を起動します。
    - マシンにデバッガがインストールされている必要があります。
* `-s` オプションをつけると，小さいパラメータで実行します。
