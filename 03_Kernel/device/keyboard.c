#include "keyboard.h"
#include "stdint.h"
#include "interrupt.h"
#include "console.h"
#include "io.h"

/* 使用转义字符定义特殊字符定义 */
#define ESC       ('\x1b')
#define BACKSPACE ('\b')
#define TAB       ('\t')
#define ENTER     ('\r')
#define DELETE    ('\x7f')

/* 不可见字符 */
#define INVISIBLE_CHAR (0)
#define CTRL_L    (INVISIBLE_CHAR)
#define CTRL_R    (INVISIBLE_CHAR)
#define SHIFT_L   (INVISIBLE_CHAR)
#define SHIFT_R   (INVISIBLE_CHAR)
#define ALT_L     (INVISIBLE_CHAR)
#define ALT_R     (INVISIBLE_CHAR)
#define CAPS_LOCK (INVISIBLE_CHAR)

/* 特殊字符的通码和断码 */
#define SHIFT_L_MAKECODE   (0x2A)
#define SHIFT_R_MAKECODE   (0x36)
#define ALT_L_MAKECODE     (0x38)
#define ALT_R_MAKECODE     (0xE038)
#define ALT_R_BREAKCODE    (0xE0B8)
#define CTRL_L_MAKECODE    (0x1D)
#define CTRL_R_MAKECODE    (0xE01D)
#define CTRL_R_BREAKCODE   (0xE09D)
#define CAPS_LOCK_MAKECODE (0x3A)

static char s_keymap[][2] = {
/* 扫描码   未与shift组合  与shift组合  */
/* ---------------------------------- */
/* 0x00 */ {0,	      0},
/* 0x01 */ {ESC,	ESC},
/* 0x02 */ {'1',	'!'},
/* 0x03 */ {'2',	'@'},
/* 0x04 */ {'3',	'#'},
/* 0x05 */ {'4',	'$'},
/* 0x06 */ {'5',	'%'},
/* 0x07 */ {'6',	'^'},
/* 0x08 */ {'7',	'&'},
/* 0x09 */ {'8',	'*'},
/* 0x0A */ {'9',	'('},
/* 0x0B */ {'0',	')'},
/* 0x0C */ {'-',	'_'},
/* 0x0D */ {'=',	'+'},
/* 0x0E */ {BACKSPACE, BACKSPACE},	
/* 0x0F */ {TAB,	TAB},
/* 0x10 */ {'q',	'Q'},
/* 0x11 */ {'w',	'W'},
/* 0x12 */ {'e',	'E'},
/* 0x13 */ {'r',	'R'},
/* 0x14 */ {'t',	'T'},
/* 0x15 */ {'y',	'Y'},
/* 0x16 */ {'u',	'U'},
/* 0x17 */ {'i',	'I'},
/* 0x18 */ {'o',	'O'},
/* 0x19 */ {'p',	'P'},
/* 0x1A */ {'[',	'{'},
/* 0x1B */ {']',	'}'},
/* 0x1C */ {ENTER,  ENTER},
/* 0x1D */ {CTRL_L, CTRL_L},
/* 0x1E */ {'a',	'A'},
/* 0x1F */ {'s',	'S'},
/* 0x20 */ {'d',	'D'},
/* 0x21 */ {'f',	'F'},
/* 0x22 */ {'g',	'G'},
/* 0x23 */ {'h',	'H'},
/* 0x24 */ {'j',	'J'},
/* 0x25 */ {'k',	'K'},
/* 0x26 */ {'l',	'L'},
/* 0x27 */ {';',	':'},
/* 0x28 */ {'\'',	'"'},
/* 0x29 */ {'`',	'~'},
/* 0x2A */ {SHIFT_L, SHIFT_L},	
/* 0x2B */ {'\\',	'|'},
/* 0x2C */ {'z',	'Z'},
/* 0x2D */ {'x',	'X'},
/* 0x2E */ {'c',	'C'},
/* 0x2F */ {'v',	'V'},
/* 0x30 */ {'b',	'B'},
/* 0x31 */ {'n',	'N'},
/* 0x32 */ {'m',	'M'},
/* 0x33 */ {',',	'<'},
/* 0x34 */ {'.',	'>'},
/* 0x35 */ {'/',	'?'},
/* 0x36	*/ {SHIFT_R, SHIFT_R},	
/* 0x37 */ {'*',	'*'},    	
/* 0x38 */ {ALT_L,   ALT_L},
/* 0x39 */ {' ',	' '},
/* 0x3A */ {CAPS_LOCK, CAPS_LOCK}
/*其它按键暂不处理*/
};

/* 扫描码是否扩展码标记 */
static bool s_extScancode_Flag = false;
/* CTRL是否按下标记 */
static bool s_CTRL_Flag = false;
/* SHIFT是否按下标记 */
static bool s_SHIFT_Flag = false;
/* ALT是否按下标记 */
static bool s_ALT_Flag = false;
/* CAPS_LOCK是否按下标记 */
static bool s_CAPS_LOCK_Flag = false;


void keyboard_interrupt_handle() {
    bool isBreakCode = false;

    uint16_t scancode = inb(KEYBOARD_BUFFER_PORT);
    /* 判断是否是扩展扫描码，是则置标记为为true，下次继续读取合并 */
    if (scancode == 0xE0) {
        s_extScancode_Flag = true;
        return;
    }
    /* 是扩展的扫描码，合并扫描码 */
    if (s_extScancode_Flag) {
        scancode = (scancode | 0xE000);
        s_extScancode_Flag = false;
    }

    isBreakCode = ((scancode & 0x0080) != 0);  // 判断扫描码是否为断码

    if (isBreakCode) {
        uint16_t makecode = scancode & 0xFF7F;
        if ((makecode == CTRL_L_MAKECODE) || (makecode == CTRL_R_MAKECODE)) {
            s_CTRL_Flag = false;
        } else if ((makecode == SHIFT_L_MAKECODE) || (makecode == SHIFT_R_MAKECODE)) {
            s_SHIFT_Flag = false;
        } else if ((makecode == ALT_L_MAKECODE) || (makecode == ALT_R_MAKECODE)) {
            s_ALT_Flag = false;
        }
        return;
    } else if ((scancode > 0x00 && scancode < 0x3b) ||
               (scancode == CTRL_R_MAKECODE) ||
               (scancode == ALT_R_MAKECODE)) {  // 只处理有效范围内的按键
        
        if (scancode == CAPS_LOCK_MAKECODE) {
            s_CAPS_LOCK_Flag = (s_CAPS_LOCK_Flag == true ? false : true);  // CAPS_LOCK 按键, 按下就会翻转状态
        }
        if ((scancode == CTRL_L_MAKECODE) || (scancode == CTRL_R_MAKECODE)) {  // CTRL 按键
            s_CTRL_Flag = true;
        }
        if ((scancode == SHIFT_L_MAKECODE) || (scancode == SHIFT_R_MAKECODE)) {  // SHIFT 按键
            s_SHIFT_Flag = true;
        }
        if ((scancode == ALT_L_MAKECODE) || (scancode == ALT_R_MAKECODE)) {  // ALT 按键
            s_ALT_Flag = true;
        }

        /* shift 组合按键 */
        uint32_t shift = 0;
        if (((s_SHIFT_Flag == true) && (s_CAPS_LOCK_Flag == false)) ||
            ((s_SHIFT_Flag == false) && (s_CAPS_LOCK_Flag == true))) {  // shift、caps_lock只有1个按下
            shift = 1;
        }

        char key_char = s_keymap[scancode][shift];
        if (key_char != INVISIBLE_CHAR) {
            console_put_char(key_char);
        }
    } else {
        console_put_str("unknown key!\n");
    }
}

void keyboard_init() {
    console_put_str("keyboard init start!\n");
    register_interrupt_handle(KEYBOARD_INTR_NUM, keyboard_interrupt_handle);
    console_put_str("keyboard init done!\n");
}