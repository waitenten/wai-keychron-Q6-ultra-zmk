/*
 * ==========================================================
 * Keychron/ZMK標準には存在しない、独立した追加モジュールです。
 *
 * 目的:
 *   ZMKの「キーが押された」イベント(zmk_keycode_state_changed)を、
 *   標準のHIDリスナー(hid_listener.c)より先に横取りし、
 *   ANSI配列の物理キーボードを、OS側の設定が「日本語(JIS)」の
 *   ままでも、キーキャップ通りの記号が入力されるように、
 *   送信予定のキーコードそのものを書き換える。
 *
 *   keymapファイル(.keymap)側は標準の &kp のままで済むため、
 *   Keychron Launcherの「キーマップ語彙変換」処理の対象にならず、
 *   Factory Reset等で消される心配がない。
 *
 * 対応表の根拠:
 *   以前 jis_* という名前でmod-morphとして実装していた14個の
 *   ビヘイビアと、完全に同じ対応関係(それぞれの bindings の
 *   中身)を、ここで再現している。
 * ==========================================================
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(jis_override, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/event_manager.h>
#include <zmk/events/keycode_state_changed.h>
#include <dt-bindings/zmk/keys.h>
#include <dt-bindings/zmk/modifiers.h>

#define SHIFT_BITS (MOD_LSFT | MOD_RSFT)

struct jis_override_rule {
    uint16_t trigger_keycode;  // 変換前(ANSIキーボード上、この位置が押された)
    bool     trigger_shift;    // このルールは「Shiftを伴う場合」のものか
    uint16_t output_keycode;   // 変換後、実際に送信する生コード
    bool     output_shift;     // 送信時、Shiftビットを立てるかどうか
};

// 以前の jis_* mod-morph 14個と、完全に同じ対応関係
static const struct jis_override_rule rules[] = {
    // GRAVE: 無変換 → ` / Shift → ~
    { GRAVE,      false, LEFT_BRACKET, true  },
    { GRAVE,      true,  EQUAL,        true  },
    // 2: Shift → @ (無変換はそのまま2なのでルール不要)
    { N2,         true,  LEFT_BRACKET, false },
    // 6: Shift → ^
    { N6,         true,  EQUAL,        false },
    // 7: Shift → &
    { N7,         true,  N6,           true  },
    // 8: Shift → *
    { N8,         true,  SINGLE_QUOTE, false },
    // 9: Shift → (
    { N9,         true,  N8,           true  },
    // 0: Shift → )
    { N0,         true,  N9,           true  },
    // -: Shift → _
    { MINUS,      true,  INT1,         true  },
    // =: 無変換 → = / Shift → +
    { EQUAL,      false, MINUS,        true  },
    { EQUAL,      true,  SEMICOLON,    true  },
    // [: 無変換 → [ / Shift → {
    { LEFT_BRACKET,  false, RIGHT_BRACKET, false },
    { LEFT_BRACKET,  true,  RIGHT_BRACKET, true  },
    // ]: 無変換 → ] / Shift → }
    { RIGHT_BRACKET, false, BACKSLASH,     false },
    { RIGHT_BRACKET, true,  BACKSLASH,     true  },
    // \: 無変換 → ¥ / Shift → |
    { BACKSLASH,  false, INT3,         false },
    { BACKSLASH,  true,  INT3,         true  },
    // ;: Shift → :
    { SEMICOLON,  true,  SINGLE_QUOTE, false },
    // ': 無変換 → ' / Shift → "
    { SINGLE_QUOTE, false, N7,         true  },
    { SINGLE_QUOTE, true,  N2,         true  },
};

#define RULES_LEN (sizeof(rules) / sizeof(rules[0]))

// 押した瞬間にどのルールを適用したかを覚えておき、
// 離す時に同じ変換を使うための、簡易な状態記録
#define ACTIVE_LEN 16
struct active_sub {
    uint16_t original_keycode;
    bool     in_use;
    uint16_t output_keycode;
    bool     output_shift;
};
static struct active_sub active[ACTIVE_LEN];

static struct active_sub *find_active(uint16_t original_keycode, bool allocate) {
    struct active_sub *free_slot = NULL;
    for (int i = 0; i < ACTIVE_LEN; i++) {
        if (active[i].in_use && active[i].original_keycode == original_keycode) {
            return &active[i];
        }
        if (!active[i].in_use && free_slot == NULL) {
            free_slot = &active[i];
        }
    }
    return allocate ? free_slot : NULL;
}

static int jis_override_listener(const zmk_event_t *eh) {
    struct zmk_keycode_state_changed *ev = as_zmk_keycode_state_changed(eh);
    if (ev == NULL) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    if (ev->usage_page != HID_USAGE_KEY) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    uint16_t original_keycode = ev->keycode;

    // GRAVEキー + Alt: 全角半角切り替え(LANG5)。以前の jis_grave の
    // Alt分岐と同じ挙動。Shift/無変換とは別枠でここだけ先に判定する。
    if (original_keycode == GRAVE) {
        if (ev->state) {
            bool alt_held = (ev->explicit_modifiers & (MOD_LALT | MOD_RALT)) != 0;
            if (alt_held) {
                struct active_sub *slot = find_active(original_keycode, true);
                if (slot != NULL) {
                    slot->in_use = true;
                    slot->original_keycode = original_keycode;
                    slot->output_keycode = LANG5;
                    slot->output_shift = false;
                }
                ev->keycode = LANG5;
                ev->explicit_modifiers &= ~(MOD_LALT | MOD_RALT | SHIFT_BITS);
                LOG_DBG("jis_override: GRAVE+Alt -> LANG5 (zenkaku/hankaku)");
                return ZMK_EV_EVENT_BUBBLE;
            }
        } else {
            struct active_sub *slot = find_active(original_keycode, false);
            if (slot != NULL && slot->output_keycode == LANG5) {
                ev->keycode = LANG5;
                ev->explicit_modifiers &= ~(MOD_LALT | MOD_RALT | SHIFT_BITS);
                slot->in_use = false;
                LOG_DBG("jis_override: GRAVE+Alt release -> LANG5");
                return ZMK_EV_EVENT_BUBBLE;
            }
        }
    }

    if (ev->state) {
        // 押した瞬間: 今のShift状態に合うルールを探す
        bool shift_held = (ev->explicit_modifiers & SHIFT_BITS) != 0;

        for (int i = 0; i < RULES_LEN; i++) {
            if (rules[i].trigger_keycode == original_keycode &&
                rules[i].trigger_shift == shift_held) {

                struct active_sub *slot = find_active(original_keycode, true);
                if (slot != NULL) {
                    slot->in_use = true;
                    slot->original_keycode = original_keycode;
                    slot->output_keycode = rules[i].output_keycode;
                    slot->output_shift = rules[i].output_shift;
                }

                ev->keycode = rules[i].output_keycode;
                if (rules[i].output_shift) {
                    ev->explicit_modifiers |= SHIFT_BITS;
                } else {
                    ev->explicit_modifiers &= ~SHIFT_BITS;
                }
                LOG_DBG("jis_override: press 0x%02X -> 0x%02X (shift=%d)",
                        original_keycode, ev->keycode, rules[i].output_shift);
                return ZMK_EV_EVENT_BUBBLE;
            }
        }
    } else {
        // 離した瞬間: 押した時と同じ変換を、覚えている記録から再利用する
        struct active_sub *slot = find_active(original_keycode, false);
        if (slot != NULL) {
            ev->keycode = slot->output_keycode;
            if (slot->output_shift) {
                ev->explicit_modifiers |= SHIFT_BITS;
            } else {
                ev->explicit_modifiers &= ~SHIFT_BITS;
            }
            slot->in_use = false;
            LOG_DBG("jis_override: release 0x%02X -> 0x%02X", original_keycode, ev->keycode);
        }
    }

    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(jis_override, jis_override_listener);
ZMK_SUBSCRIPTION(jis_override, zmk_keycode_state_changed);
