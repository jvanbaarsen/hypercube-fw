/**
 * Marlin 3D Printer Firmware
 * Copyright (c) 2019 MarlinFirmware [https://github.com/MarlinFirmware/Marlin]
 *
 * Based on Sprinter and grbl.
 * Copyright (c) 2011 Camiel Gubbels / Erik van der Zalm
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

//
// Main Menu
//

#include "../../inc/MarlinConfigPre.h"

#if HAS_LCD_MENU

#include "menu.h"
#include "../../module/temperature.h"
#include "../../gcode/queue.h"
#include "../../module/printcounter.h"
#include "../../module/stepper.h"
#include "../../sd/cardreader.h"

#if HAS_GAMES && DISABLED(LCD_INFO_MENU)
  #include "game/game.h"
#endif

#define MACHINE_CAN_STOP (EITHER(SDSUPPORT, HOST_PROMPT_SUPPORT) || defined(ACTION_ON_CANCEL))
#define MACHINE_CAN_PAUSE (ANY(SDSUPPORT, HOST_PROMPT_SUPPORT, PARK_HEAD_ON_PAUSE) || defined(ACTION_ON_PAUSE))

#if MACHINE_CAN_STOP
  void menu_abort_confirm() {
    do_select_screen(GET_TEXT(MSG_BUTTON_STOP), GET_TEXT(MSG_BACK), ui.abort_print, ui.goto_previous_screen, GET_TEXT(MSG_STOP_PRINT), nullptr, PSTR("?"));
  }
#endif // MACHINE_CAN_STOP

#if ENABLED(PRUSA_MMU2)
  #include "../../lcd/menu/menu_mmu2.h"
#endif

void menu_tune();
void menu_motion();
void menu_temperature();
void menu_configuration();

#if ENABLED(CUSTOM_USER_MENUS)
  void menu_user();
#endif

#if ENABLED(ADVANCED_PAUSE_FEATURE)
  void _menu_temp_filament_op(const PauseMode, const int8_t);
  void menu_change_filament();
#endif

#if ENABLED(LCD_INFO_MENU)
  void menu_info();
#endif

#if ENABLED(LED_CONTROL_MENU)
  void menu_led();
#endif

#if HAS_CUTTER
  #include "../../feature/spindle_laser.h"
  void menu_spindle_laser();
#endif

#if ENABLED(MIXING_EXTRUDER)
  void menu_mixer();
#endif

#if HAS_SERVICE_INTERVALS
  #if SERVICE_INTERVAL_1 > 0
    void menu_service1();
  #endif
  #if SERVICE_INTERVAL_2 > 0
    void menu_service2();
  #endif
  #if SERVICE_INTERVAL_3 > 0
    void menu_service3();
  #endif
#endif

void menu_main() {
  START_MENU();
  BACK_ITEM(MSG_WATCH);

  const bool busy = printingIsActive()
    #if ENABLED(SDSUPPORT)
      , card_detected = card.isMounted()
      , card_open = card_detected && card.isFileOpen()
    #endif
  ;

  if (busy) {
    #if MACHINE_CAN_PAUSE
      ACTION_ITEM(MSG_PAUSE_PRINT, ui.pause_print);
    #endif
    #if MACHINE_CAN_STOP
      SUBMENU(MSG_STOP_PRINT, menu_abort_confirm);
    #endif
    SUBMENU(MSG_TUNE, menu_tune);
  }
  else {
    #if !HAS_ENCODER_WHEEL && ENABLED(SDSUPPORT)
      //
      // Autostart
      //
      #if ENABLED(MENU_ADDAUTOSTART)
        if (!busy) ACTION_ITEM(MSG_AUTOSTART, card.beginautostart);
      #endif

      if (card_detected) {
        if (!card_open) {
          SUBMENU(MSG_MEDIA_MENU, menu_media);
          MENU_ITEM(gcode,
            #if PIN_EXISTS(SD_DETECT)
              MSG_CHANGE_MEDIA, PSTR("M21")
            #else
              MSG_RELEASE_MEDIA, PSTR("M22")
            #endif
          );
        }
      }
      else {
        #if PIN_EXISTS(SD_DETECT)
          ACTION_ITEM(MSG_NO_MEDIA, nullptr);
        #else
          GCODES_ITEM(MSG_INIT_MEDIA, PSTR("M21"));
          ACTION_ITEM(MSG_MEDIA_RELEASED, nullptr);
        #endif
      }
    #endif // !HAS_ENCODER_WHEEL && SDSUPPORT

    #if MACHINE_CAN_PAUSE
      if (printingIsPaused()) ACTION_ITEM(MSG_RESUME_PRINT, ui.resume_print);
    #endif

    SUBMENU(MSG_MOTION, menu_motion);
  }

  #if HAS_CUTTER
    SUBMENU(MSG_CUTTER(MENU), menu_spindle_laser);
  #endif

  SUBMENU(MSG_TEMPERATURE, menu_temperature);

  #if ENABLED(MIXING_EXTRUDER)
    SUBMENU(MSG_MIXER, menu_mixer);
  #endif

  #if ENABLED(MMU2_MENUS)
    if (!busy) SUBMENU(MSG_MMU2_MENU, menu_mmu2);
  #endif

  SUBMENU(MSG_CONFIGURATION, menu_configuration);

  #if ENABLED(CUSTOM_USER_MENUS)
    #ifdef CUSTOM_USER_MENU_TITLE
      #undef MSG_USER_MENU
      #define MSG_USER_MENU CUSTOM_USER_MENU_TITLE
    #endif
    SUBMENU(MSG_USER_MENU, menu_user);
  #endif

  #if ENABLED(ADVANCED_PAUSE_FEATURE)
    #if E_STEPPERS == 1 && DISABLED(FILAMENT_LOAD_UNLOAD_GCODES)
      if (thermalManager.targetHotEnoughToExtrude(active_extruder))
        GCODES_ITEM(MSG_FILAMENTCHANGE, PSTR("M600 B0"));
      else
        SUBMENU(MSG_FILAMENTCHANGE, [](){ _menu_temp_filament_op(PAUSE_MODE_LOAD_FILAMENT, 0); });
    #else
      SUBMENU(MSG_FILAMENTCHANGE, menu_change_filament);
    #endif
  #endif

  #if ENABLED(LCD_INFO_MENU)
    SUBMENU(MSG_INFO_MENU, menu_info);
  #endif

  #if ENABLED(LED_CONTROL_MENU)
    SUBMENU(MSG_LED_CONTROL, menu_led);
  #endif

  //
  // Switch power on/off
  //
  #if HAS_POWER_SWITCH
    if (powersupply_on)
      GCODES_ITEM(MSG_SWITCH_PS_OFF, PSTR("M81"));
    else
      GCODES_ITEM(MSG_SWITCH_PS_ON, PSTR("M80"));
  #endif

  #if HAS_ENCODER_WHEEL && ENABLED(SDSUPPORT)
    //
    // Autostart
    //
    #if ENABLED(MENU_ADDAUTOSTART)
      if (!busy) ACTION_ITEM(MSG_AUTOSTART, card.beginautostart);
    #endif

    if (card_detected) {
      if (!card_open) {
        MENU_ITEM(gcode,
          #if PIN_EXISTS(SD_DETECT)
            MSG_CHANGE_MEDIA, PSTR("M21")
          #else
            MSG_RELEASE_MEDIA, PSTR("M22")
          #endif
        );
        SUBMENU(MSG_MEDIA_MENU, menu_media);
      }
    }
    else {
      #if PIN_EXISTS(SD_DETECT)
        ACTION_ITEM(MSG_NO_MEDIA, nullptr);
      #else
        GCODES_ITEM(MSG_INIT_MEDIA, PSTR("M21"));
        ACTION_ITEM(MSG_MEDIA_RELEASED, nullptr);
      #endif
    }
  #endif // HAS_ENCODER_WHEEL && SDSUPPORT

  #if HAS_SERVICE_INTERVALS
    #if SERVICE_INTERVAL_1 > 0
      SUBMENU_P(PSTR(SERVICE_NAME_1), menu_service1);
    #endif
    #if SERVICE_INTERVAL_2 > 0
      SUBMENU_P(PSTR(SERVICE_NAME_2), menu_service2);
    #endif
    #if SERVICE_INTERVAL_3 > 0
      SUBMENU_P(PSTR(SERVICE_NAME_3), menu_service3);
    #endif
  #endif

  #if HAS_GAMES && DISABLED(LCD_INFO_MENU)
    #if ENABLED(GAMES_EASTER_EGG)
      SKIP_ITEM();
      SKIP_ITEM();
    #endif
    SUBMENU(MSG_GAMES, (
      #if HAS_GAME_MENU
        menu_game
      #elif ENABLED(MARLIN_BRICKOUT)
        brickout.enter_game
      #elif ENABLED(MARLIN_INVADERS)
        invaders.enter_game
      #elif ENABLED(MARLIN_SNAKE)
        snake.enter_game
      #elif ENABLED(MARLIN_MAZE)
        maze.enter_game
      #endif
    ));
  #endif

  END_MENU();
}

#endif // HAS_LCD_MENU
