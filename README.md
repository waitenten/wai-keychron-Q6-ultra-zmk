# Keychron Q6 Ultra (ANSI) — 個人用 zmk-config

board: `rtl8762gku`
shield: `keychron_q6_ultra_ansi`

## 使い方
1. このフォルダ一式を、GitHub上の新規リポジトリ(自分のアカウント配下)にアップロード/コミットする。
2. リポジトリの Settings > Actions で Actions を有効化する(fork直後は無効になっていることが多い)。
3. `config/keychron_q6_ultra_ansi.keymap` を編集してコミットすると、Actionsタブでビルドが走り、
   Artifacts に `firmware.zip`(中に `.uf2`)が生成される。
4. https://nickcoutsos.github.io/keymap-editor/ を開き、GitHub連携でこのリポジトリを指定すると、
   `config/keychron_q6_ultra_ansi.keymap` をGUIで編集できる(はず)。

## 著作権表記について
- Keychron由来のファイル(keymap/overlay/conf等)は、冒頭のCopyright表記・SPDXライセンス表記を含め、削除・改変せずそのまま使用しています。
- リポジトリ直下に、Keychron/zmk本体のMIT LICENSEファイルもそのまま同梱しています。
- 独自に追記した箇所(JIS/ANSI記号補正のbehaviors、layer_twoの一部bindings)には、
  「ここから下はKeychron標準ファームウェアには存在しない追記」であることを
  コメントで明記し、Keychronが書いた部分との区別ができるようにしています。

## 課題2(JIS/ANSI記号補正)の実装方針
当初検討していた「Keyboard Quantizer(せきごん氏)のQMK Key Overrideをそのまま移植する」方針は中止し、
課題1の延長として、keymapファイル内に直接組み込みました。
- 実装場所: `behaviors` ブロック内に `jis_*` という名前のmod-morphビヘイビアを14個追加
- 適用場所: `layer_two`(win用レイヤー)の該当キーのみ置き換え(GRAVE, N2, N6-N0, MINUS, EQUAL, LBKT, RBKT, BSLH, SEMI, SQT)
- ランタイムのトグル切り替えは実装していません(常時JIS OS環境での使用を前提とした固定対応)
- 使用しているキーコード名(AT, CARET, INT3 等)はすべてZMK本体`keys.h`で標準定義済みのものです

## 既知の注意点(実データ確認済み)
- `keychron_q6_ultra_ansi.overlay` は `zmk,matrix-transform`(旧方式)でキー配列を定義しており、
  ZMK Studio/Keymap Editorの新しい「physical layout(keys プロパティ付き)」形式には
  なっていません。そのためKeymap Editor側の「見た目のキーボード配列(グラフィカルなキー配置)」が
  正しく自動生成されない可能性があります。ただし、これは表示上の問題であり、
  裏側のdevicetree解析・編集(実際の各キーへの割当変更)自体は動作する見込みです。
  この点は実際に接続してみないと確定できないため、次のステップで検証が必要です。
- `&uc`(user_custom)、`&kc`(keychron独自ビヘイビア)、タップダンス`&td1`、
  マクロ(`siri`、`lm`など)、`combos`ブロックはKeychron独自定義です。
  Keymap Editorはこれらを「未対応ビヘイビア」として認識せず、
  既存の記述をそのまま保持するはずです(削除・改変はされない設計)。
