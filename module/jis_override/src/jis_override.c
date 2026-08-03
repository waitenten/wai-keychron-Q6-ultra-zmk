/*
 * ==========================================================
 * Keychron/ZMK標準には存在しない、独立した追加モジュールです。
 *
 * 目的:
 *   ZMKの「キーが押された」イベント(zmk_keycode_state_changed)を、
 *   標準のHIDリスナー(hid_listener.c、実際にPCへ送る信号を
 *   組み立てる処理)より先に横取りし、特定の条件(例: 2キー+Shift)
 *   に一致した場合、送信予定のキーコードそのものを書き換える。
 *
 *   これにより、keymapファイル(.keymap)側は標準の &kp のままで
 *   済むため、Keychron Launcherの「キーマップ語彙変換」処理の
 *   対象にならず、Factory Reset等で消される心配がない。
 *
 * 現状: まずは動作確認のため「2キー + Shift → @ (JIS OS上)」の
 *       1件だけを実装している。動作確認が取れ次第、残り13件を
 *       追加する。
 * ==========================================================
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(jis_override, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/event_manager.h>
#include <zmk/events/keycode_state_changed.h>
#include <dt-bindings/zmk/keys.h>
#include <dt-bindings/zmk/modifiers.h>

static int jis_override_listener(const zmk_event_t *eh) {
    struct zmk_keycode_state_changed *ev = as_zmk_keycode_state_changed(eh);
    if (ev == NULL) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    // キーボード用途(HID_USAGE_KEY = 0x07)以外は対象外
    if (ev->usage_page != HID_USAGE_KEY) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    // テストケース: 2キー(N2)が、Shiftを伴って押された場合
    //   -> 生コード LEFT_BRACKET(Shiftは外す)に置き換える
    //   -> JIS OS側では、これが「@」として表示される
    if (ev->keycode == N2 && (ev->explicit_modifiers & (MOD_LSFT | MOD_RSFT))) {
        LOG_DBG("jis_override: N2+Shift -> LEFT_BRACKET (raw)");
        ev->keycode = LEFT_BRACKET;
        ev->explicit_modifiers &= ~(MOD_LSFT | MOD_RSFT);
        ev->implicit_modifiers &= ~(MOD_LSFT | MOD_RSFT);
    }

    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(jis_override, jis_override_listener);
ZMK_SUBSCRIPTION(jis_override, zmk_keycode_state_changed);
