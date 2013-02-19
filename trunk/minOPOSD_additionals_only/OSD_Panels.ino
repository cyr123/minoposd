//
//
//

/*

refactoring started
there is more refactoring necessary because there are too much side effects

TODO:
	refactor:
		panWarn
		panOff
		panSetup
		
	renaming:
		waitingMAVBeats
		
	maybe port panCallsign
	
*/


#include "OSD_Config.h"

#ifdef FLIGHT_BATT_ON_MINIMOSD
#include "FlightBatt.h"
#endif

#ifdef PACKETRXOK_ON_MINIMOSD
#include "PacketRxOk.h"
#endif


#define LOWEST_SETUP_MENU	2	// lowest shown setup menue item

#ifndef FLIGHT_BATT_ON_MINIMOSD
#define HIGHEST_SETUP_MENU	2	// highest shown setup menue item
#else
#define HIGHEST_SETUP_MENU	11	// highest shown setup menue item
#endif

#define MAX_WARNING		3	// change this if you add more warnings




/******* STARTUP PANEL *******/

void startPanels() {
    osd.clear();
    panLogo();		// display logo  
    set_converts();	// initialize the units
}


/******* PANELS - POSITION *******/

void writePanels() {
    if (uavtalk_state() == TELEMETRYSTATS_STATE_CONNECTED) {
        if (waitingMAVBeats == 1) {
            osd.clear();
	    waitingMAVBeats = 0;
        }
        if (ch_toggle > 3) panOff();										// This must be first so you can always toggle
        if (osd_set == 0) {											// setup panel is called in the else at the end
            if (panel != npanels) {
                if (ISd(panel,Warn_BIT))	panWarn(panWarn_XY[0][panel], panWarn_XY[1][panel]);		// this must be here so warnings are always checked
		
		// these GPS related panels are active under all circumstances
                if (ISa(panel,GPSats_BIT))	panGPSats(panGPSats_XY[0][panel], panGPSats_XY[1][panel]);	// number of visible sats
                if (ISa(panel,GPL_BIT))		panGPL(panGPL_XY[0][panel], panGPL_XY[1][panel]);		// sat fix type
		
		// these GPS related panels are active if GPS was valid before
		if (osd_got_home) {
		    if (ISa(panel,GPS_BIT))	panGPS(panGPS_XY[0][panel], panGPS_XY[1][panel]);		// Lat & Lon
		    if (ISb(panel,HDis_BIT))	panHomeDis(panHomeDis_XY[0][panel], panHomeDis_XY[1][panel]);
                    if (ISb(panel,HDir_BIT))	panHomeDir(panHomeDir_XY[0][panel], panHomeDir_XY[1][panel]);
		}
		
		// these GPS related panels are active if GPS was valid before and we have a sat fix
		if (osd_got_home && osd_fix_type > 1) {
                    if (ISc(panel,Halt_BIT))	panHomeAlt(panHomeAlt_XY[0][panel], panHomeAlt_XY[1][panel]);
                    if (ISc(panel,Alt_BIT))	panAlt(panAlt_XY[0][panel], panAlt_XY[1][panel]);
                    if (ISc(panel,Vel_BIT))	panVel(panVel_XY[0][panel], panVel_XY[1][panel]);
                    if (ISd(panel,Climb_BIT))	panClimb(panClimb_XY[0][panel], panClimb_XY[1][panel]);
                    if (ISb(panel,Head_BIT))	panHeading(panHeading_XY[0][panel], panHeading_XY[1][panel]);
                    if (ISb(panel,Rose_BIT))	panRose(panRose_XY[0][panel], panRose_XY[1][panel]);
		}
		
                if (ISd(panel,RSSI_BIT))	panRSSI(panRSSI_XY[0][panel], panRSSI_XY[1][panel]);
                if (ISa(panel,Rol_BIT))		panRoll(panRoll_XY[0][panel], panRoll_XY[1][panel]);
                if (ISa(panel,Pit_BIT))		panPitch(panPitch_XY[0][panel], panPitch_XY[1][panel]);
                if (ISc(panel,Thr_BIT))		panThr(panThr_XY[0][panel], panThr_XY[1][panel]);
                if (ISc(panel,FMod_BIT))	panFlightMode(panFMod_XY[0][panel], panFMod_XY[1][panel]);
                if (ISa(panel,BatA_BIT))	panBatt_A(panBatt_A_XY[0][panel], panBatt_A_XY[1][panel]);
                if (ISc(panel,CurA_BIT))	panCur_A(panCur_A_XY[0][panel], panCur_A_XY[1][panel]);
                if (ISa(panel,Bp_BIT))		panBatteryPercent(panBatteryPercent_XY[0][panel], panBatteryPercent_XY[1][panel]);
                if (ISb(panel,Time_BIT))	panTime(panTime_XY[0][panel], panTime_XY[1][panel]);
                if (ISc(panel,Hor_BIT))		panHorizon(panHorizon_XY[0][panel], panHorizon_XY[1][panel]);
            } else {												// panel == npanels
                if (ISd(0,Warn_BIT))		panWarn(panWarn_XY[0][0], panWarn_XY[1][0]);			// this must be here so warnings are always checked
            }
        } else {												// if (osd_on > 0)
            panSetup();
        }
    } else {													// no telemetry communication
        if (waitingMAVBeats == 0) {
            osd.clear();
        }
        waitingMAVBeats = 1;
        panWaitCom(5,10);
    }

#ifdef membug
    // OSD debug for development
    osd.setPanel(13,4);
    osd.openPanel();
    osd.printf("%i",freeMem()); 
    osd.closePanel();
#endif
}


/******* PANELS - SPECIALS *******/

/******************************************************************/
// Panel  : panWarn
// Needs  : X, Y locations
// Output : 
// TODO   : REFACTOR
/******************************************************************/
void panWarn(int first_col, int first_line) {
    char* warning_string;
    int x;

    if (millis() > text_timer) {				// if the text or blank text has been shown for a while
        if (warning_type != 0) {				// there was a warning, so we now blank it out 1s
            last_warning = warning_type;			// save the warning type for cycling
            warning_type = 0;					// blank the text
            warning = 1;
            warning_timer = millis();            
	    text_timer = millis() + 1000;			// clear text 1 sec
        } else {
            if ((millis() - 10000) > warning_timer ) warning = 0;

            x = last_warning;					// start the warning checks where we left it last time
            while (warning_type == 0) {				// cycle through the warning checks
                x++;
                if (x > MAX_WARNING) x = 1;
                switch (x) {
                case 1:						// NO GPS FIX
								// to allow flying in the woods (what I really like) without this annoying warning, 
								// this warning is only shown if GPS was valid before (osd_got_home)
                    if ((osd_fix_type) < 2 && osd_got_home) warning_type = x;
                    break;
                case 2:						// LOW BATT
#ifdef FLIGHT_BATT_ON_MINIMOSD
                    if (osd_vbat_A < battv/10.0) warning_type = x;
#else
                    if (osd_vbat_A < float(battv)/10.0 || osd_battery_remaining_A < batt_warn_level) warning_type = x;
#endif
                    break;
                case 3:						// LOW RSSI
#ifdef PACKETRXOK_ON_MINIMOSD
		    rssi = PacketRxOk_get();
#endif
                    if (rssi < rssi_warn_level && rssi != -99 && !rssiraw_on) warning_type = x;
                    break;
                }
                if (x == last_warning) break;			// we've done a full cycle
            }
	    if (warning_type != 0) {
		text_timer = millis() + 1000;			// show warning 1 sec if there is any
	    }							// if not, we do not want the 1s delay, so a new error shows up immediately
        }
	
        if (motor_armed == 0) {
	    warning_string = "  DISARMED  ";
        } else {
            switch (warning_type) { 
            case 0:
                warning_string = "            ";
                break;   
            case 1:
                warning_string = " NO GPS FIX ";
                break;
            case 2:
                warning_string = "  BATT LOW  ";
                break;
            case 3:
                warning_string = "  RSSI LOW  ";
                break;
            }
        }

	osd.setPanel(first_col, first_line);
	osd.openPanel();
	
        if (warning == 1) { 
            if (panel == 1) osd.clear();
            panel = 0;						// switch to first panel if there is a warning                  
        }
	
        osd.printf("%s", warning_string);
	osd.closePanel();
    }
}


/******************************************************************/
// Panel  : panOff
// Needs  : X, Y locations
// Output : OSD off
// TODO   : REFACTOR
/******************************************************************/
void panOff(){
    if (ch_toggle == 4) {
        if ((osd_mode != FLIGHTSTATUS_FLIGHTMODE_AUTOTUNE) && (osd_mode != FLIGHTSTATUS_FLIGHTMODE_POSITIONHOLD)) {
            if (osd_off_switch != osd_mode) { 
                osd_off_switch = osd_mode;
                osd_switch_time = millis();

                if (osd_off_switch == osd_switch_last) {
                    switch (panel) {
			case 0:
                            panel = 1;                                                        
                            if (millis() <= 60000) {
                                osd_set = 1;
                            } else {
                                osd_set = 0;
                            }                            
                           break;
			case 1:
                            panel = npanels;
                            osd_set = 0;                            
                           break;
			case npanels:
                            panel = 0;
                            break;
                    }
                    osd.clear();
                }
            }
            if ((millis() - osd_switch_time) > 2000) {
                osd_switch_last = osd_mode;
            }
        }
    }
    else {
        if (ch_toggle == 5) ch_raw = osd_chan5_raw;
        else if (ch_toggle == 6) ch_raw = osd_chan6_raw;
        else if (ch_toggle == 7) ch_raw = osd_chan7_raw;
        else if (ch_toggle == 8) ch_raw = osd_chan8_raw;

        if (switch_mode == 0) {
            if (ch_raw > 1800) {
                if (millis() <= 60000) {
                    osd_set = 1;
                }
                else if (osd_set != 1 && warning != 1) {
                    osd.clear();
                }
                panel = npanels; //off panel
            }
            else if (ch_raw < 1200 && panel != 0) {								// first panel
                osd_set = 0;
                osd.clear();
                panel = 0;
            }
            else if (ch_raw >= 1200 && ch_raw <= 1800 && setup_menu != 6 && panel != 1 && warning != 1) {	// second panel
                osd_set = 0;
                osd.clear();
                panel = 1;
            }        
        } else {

            if (ch_raw > 1200)
                if (millis() <= 60000 && osd_set != 1) {
                    if (osd_switch_time + 1000 < millis()) {
                        osd_set = 1;
                        osd_switch_time = millis();
                    }
                } else {
                    if (osd_switch_time + 1000 < millis()) {
                        osd_set = 0;
                        osd.clear();
                        if (panel == npanels) {
                            panel = 0;
                        } else {
                            panel++;
                        }
                        if (panel > 1) panel = npanels;
                        osd_switch_time = millis();
                    }
                }
        }    
    }
}


/******************************************************************/
// Panel  : panSetup
// Needs  : Nothing, uses whole screen
// Output : The settings menu
// TODO   : REFACTOR
/******************************************************************/
void panSetup() {
    int delta = 100;

    if (millis() > text_timer) {
        text_timer = millis() + 500;

        osd.clear();
        osd.setPanel(5, 3);
        osd.openPanel();
	
	osd.printf_P(PSTR("Setup screen|||"));

        if (chan1_raw_middle == 0 && chan2_raw_middle == 0) {
            chan1_raw_middle = chan1_raw;
            chan2_raw_middle = chan2_raw;
        }

        if ((chan2_raw - 100) > chan2_raw_middle ) setup_menu++;
        else if ((chan2_raw + 100) < chan2_raw_middle ) setup_menu--;
        if (setup_menu < LOWEST_SETUP_MENU) setup_menu = LOWEST_SETUP_MENU;
        else if (setup_menu > HIGHEST_SETUP_MENU) setup_menu = HIGHEST_SETUP_MENU;

        switch (setup_menu) {
            case 2:
                osd.printf_P(PSTR("Battery warning "));
                osd.printf("%3.1f%c", float(battv)/10.0 , 0x76, 0x20);
                battv = change_val(battv, battv_ADDR);
                break;
#ifdef FLIGHT_BATT_ON_MINIMOSD
            case 5:
		delta /= 10;
            case 4:
		delta /= 10;
            case 3:
		// volt_div_ratio
                osd.printf_P(PSTR("Calibrate|measured volt: "));
                osd.printf("%c%5.2f%c", 0xE2, (float)osd_vbat_A, 0x8E);
                osd.printf("||volt div ratio:  %5i", volt_div_ratio);
                volt_div_ratio = change_int_val(volt_div_ratio, volt_div_ratio_ADDR, delta);
                break;
            case 8:
		delta /= 10;
            case 7:
		delta /= 10;
            case 6:
		// curr_amp_offset
                osd.printf_P(PSTR("Calibrate|measured amp:  "));
                osd.printf("%c%5.2f%c", 0xE2, osd_curr_A * .01, 0x8F);
                osd.printf("||amp offset:      %5i", curr_amp_offset);
                curr_amp_offset = change_int_val(curr_amp_offset, curr_amp_offset_ADDR, delta);
                break;
            case 11:
		delta /= 10;
            case 10:
		delta /= 10;
            case 9:
		// curr_amp_per_volt
                osd.printf_P(PSTR("Calibrate|measured amp:  "));
                osd.printf("%c%5.2f%c", 0xE2, osd_curr_A * .01, 0x8F);
                osd.printf("||amp per volt:    %5i", curr_amp_per_volt);
                curr_amp_per_volt = change_int_val(curr_amp_per_volt, curr_amp_per_volt_ADDR, delta);
                break;
#endif
        }
    }
    osd.closePanel();
}


int change_int_val(int value, int address, int delta)
{
    int value_old = value;
    
    osd.printf( "|                   ");
    switch (delta) {
        case 100:
            osd.printf_P(PSTR("\x5E"));
	break;
        case 10:
            osd.printf_P(PSTR(" \x5E"));
	break;
        case 1:
            osd.printf_P(PSTR("  \x5E"));
	break;
    }
		
    if (chan1_raw > chan1_raw_middle + 100) value -= delta;
    if (chan1_raw < chan1_raw_middle - 100) value += delta;

    if (value != value_old && osd_set) {
	EEPROM.write(address, value&0xff);
	EEPROM.write(address+1, (value>>8)&0xff);
    }
    return value;
}


int change_val(int value, int address)
{
    uint8_t value_old = value;
    if (chan1_raw > chan1_raw_middle + 100) value--;
    if (chan1_raw  < chan1_raw_middle - 100) value++;

    if (value != value_old && osd_set) EEPROM.write(address, value);
    return value;
}


//------------------ Panel: Startup OSD LOGO -------------------------------
void panLogo(){
    osd.setPanel(3, 5);
    osd.openPanel();
    osd.printf_P(PSTR("\x20\x20\x20\x20\x20\xba\xbb\xbc\xbd\xbe|\x20\x20\x20\x20\x20\xca\xcb\xcc\xcd\xce|minOPOSD 1.2.1"));
#ifdef PACKETRXOK_ON_MINIMOSD
    osd.printf_P(PSTR(" PRxOk"));
#endif
#ifdef ANALOG_RSSI_ON_MINIMOSD
    osd.printf_P(PSTR(" ARSSI"));
#endif
#ifdef JR_SPECIALS
    osd.printf_P(PSTR(" JRS"));
#endif
    osd.closePanel();
}


//------------------ Panel: Waiting for UAVTalk comm -------------------------------
void panWaitCom(int first_col, int first_line){
//    panLogo();		// this is useless if we lost communication in flight
    osd.setPanel(first_col, first_line);
    osd.openPanel();
    osd.printf_P(PSTR("Waiting for|UAVTalk comm . . . . "));
    osd.closePanel();
}




/******* PANELS - DEFINITION *******/


/******************************************************************/
// Panel  : panBoot
// Needs  : X, Y locations
// Output : Booting up text and empty bar after that
/******************************************************************/
void panBoot(int first_col, int first_line){
    osd.setPanel(first_col, first_line);
    osd.openPanel();
    osd.printf_P(PSTR("Booting up:\xed\xf2\xf2\xf2\xf2\xf2\xf2\xf2\xf3")); 
    osd.closePanel();
}


/******************************************************************/
// Panel  : panGPSats
// Needs  : X, Y locations
// Output : 1 symbol and number of locked satellites
/******************************************************************/
void panGPSats(int first_col, int first_line){
    osd.setPanel(first_col, first_line);
    osd.openPanel();
    osd.printf("%c%2i", 0x0f, osd_satellites_visible);
    osd.closePanel();
}


/******************************************************************/
// Panel  : panGPL
// Needs  : X, Y locations
// Output : 1 static symbol with changing FIX symbol
/******************************************************************/
void panGPL(int first_col, int first_line){
    char* gps_str;
    
    osd.setPanel(first_col, first_line);
    osd.openPanel();
    if (osd_fix_type == 0 || osd_fix_type == 1) gps_str = "\x10\x20";
    else if (osd_fix_type == 2 || osd_fix_type == 3) gps_str = "\x11\x20";
    osd.printf("%s", gps_str);
#ifdef JR_SPECIALS	// I use this place for debug info
    osd.printf("%02x", op_alarm);
#endif
    osd.closePanel();
}


/******************************************************************/
// Panel  : panGPS
// Needs  : X, Y locations
// Output : two row numeric value of current GPS location with LAT/LON symbols
/******************************************************************/
void panGPS(int first_col, int first_line){
    osd.setPanel(first_col, first_line);
    osd.openPanel();
#ifdef JR_SPECIALS	// I like it more one row style
    osd.printf("%c%11.6f    %c%11.6f", 0x83, (double)osd_lat, 0x84, (double)osd_lon);
#else
    osd.printf("%c%11.6f|%c%11.6f", 0x83, (double)osd_lat, 0x84, (double)osd_lon);
#endif
    osd.closePanel();
}


/******************************************************************/
// Panel  : panHomeDis
// Needs  : X, Y locations
// Output : Distance to home
/******************************************************************/
void panHomeDis(int first_col, int first_line){
    osd.setPanel(first_col, first_line);
    osd.openPanel();
    osd.printf("%c%5.0f%c", 0x1F, (double)((osd_home_distance) * converth), high);
    osd.closePanel();
}


/******************************************************************/
// Panel  : panHomeDir
// Needs  : X, Y locations
// Output : 2 symbols that are combined as one arrow, shows direction to home
/******************************************************************/
void panHomeDir(int first_col, int first_line){
    osd.setPanel(first_col, first_line);
    osd.openPanel();
    showArrow((uint8_t)osd_home_direction, 0);
    osd.closePanel();
}


/******************************************************************/
// Panel  : panHomeAlt
// Needs  : X, Y locations
// Output : Hom altidude
/******************************************************************/
void panHomeAlt(int first_col, int first_line){
    osd.setPanel(first_col, first_line);
    osd.openPanel();
    osd.printf("%c%5.0f%c", 0xE7, (double)((osd_alt - osd_home_alt) * converth), high);
    osd.closePanel();
}


/******************************************************************/
// Panel  : panAlt
// Needs  : X, Y locations
// Output : Altidude
/******************************************************************/
void panAlt(int first_col, int first_line){
    osd.setPanel(first_col, first_line);
    osd.openPanel();
    osd.printf("%c%5.0f%c",0xE6, (double)(osd_alt * converth), high);
    osd.closePanel();
}


/******************************************************************/
// Panel  : panVel
// Needs  : X, Y locations
// Output : Velocity 
/******************************************************************/
void panVel(int first_col, int first_line){
    osd.setPanel(first_col, first_line);
    osd.openPanel();
    osd.printf("%c%3.0f%c", 0xE9, (double)(osd_groundspeed * converts), spe);
    osd.closePanel();
}


/******************************************************************/
// Panel  : panClimb
// Needs  : X, Y locations
// Output : Climb Rate
/******************************************************************/
void panClimb(int first_col, int first_line){
    osd.setPanel(first_col, first_line);
    osd.openPanel();
    osd.printf("%c%3.0f%c", 0x16, (double)(osd_climb), 0x88);
    osd.closePanel();
}


/******************************************************************/
// Panel  : panHeading
// Needs  : X, Y locations
// Output : Symbols with numeric compass heading value
/******************************************************************/
void panHeading(int first_col, int first_line){
    osd.setPanel(first_col, first_line);
    osd.openPanel();
    osd.printf("%4.0f%c", (double)osd_heading, 0xb0);
    osd.closePanel();
}


/******************************************************************/
// Panel  : panRose
// Needs  : X, Y locations
// Output : a dynamic compass rose that changes along the heading information
/******************************************************************/
void panRose(int first_col, int first_line){
    osd.setPanel(first_col, first_line);
    osd.openPanel();
    //osd_heading  = osd_yaw;
    //if (osd_yaw < 0) osd_heading = 360 + osd_yaw;
    osd.printf("%s|%c%s%c", "\x20\xc0\xc0\xc0\xc0\xc0\xc7\xc0\xc0\xc0\xc0\xc0\x20", 0xd0, buf_show, 0xd1);
    osd.closePanel();
}


/******************************************************************/
// Panel  : panRSSI
// Needs  : X, Y locations
// Output : RSSI %
/******************************************************************/
void panRSSI(int first_col, int first_line) {
    osd.setPanel(first_col, first_line);
    osd.openPanel();
#ifdef PACKETRXOK_ON_MINIMOSD
    PacketRxOk_print();
#else
    rssi = (int16_t)osd_rssi;
    if (!rssiraw_on) rssi = (int16_t)((float)(rssi - rssipersent)/(float)(rssical-rssipersent)*100.0f);
    if (rssi < -99) rssi = -99;
    osd.printf("%c%3i%c", 0xE1, rssi, 0x25);
#endif
    osd.closePanel();
}


/******************************************************************/
// Panel  : panRoll
// Needs  : X, Y locations
// Output : -+ value of current Roll from vehicle with degree symbols and roll symbol
/******************************************************************/
void panRoll(int first_col, int first_line){
    osd.setPanel(first_col, first_line);
    osd.openPanel();
    osd.printf("%4i%c%c", osd_roll, 0xb0, 0xb2);
    osd.closePanel();
}


/******************************************************************/
// Panel  : panPitch
// Needs  : X, Y locations
// Output : -+ value of current Pitch from vehicle with degree symbols and pitch symbol
/******************************************************************/
void panPitch(int first_col, int first_line){
    osd.setPanel(first_col, first_line);
    osd.openPanel();
    osd.printf("%4i%c%c", osd_pitch, 0xb0, 0xb1);
    osd.closePanel();
}

  
/******************************************************************/
// Panel  : panThr
// Needs  : X, Y locations
// Output : Throttle 
/******************************************************************/
void panThr(int first_col, int first_line){
    osd.setPanel(first_col, first_line);
    osd.openPanel();
    osd.printf("%c%3.0i%c", 0x87, osd_throttle, 0x25);
    osd.closePanel();
}


/******************************************************************/
// Panel  : panFlightMode 
// Needs  : X, Y locations
// Output : current flight modes
/******************************************************************/
void panFlightMode(int first_col, int first_line){
    char* mode_str="";
    
    osd.setPanel(first_col, first_line);
    osd.openPanel();
    if      (osd_mode == FLIGHTSTATUS_FLIGHTMODE_MANUAL         ) mode_str = "man";	// MANUAL
    else if (osd_mode == FLIGHTSTATUS_FLIGHTMODE_STABILIZED1    ) mode_str = "st1";	// STABILIZED1
    else if (osd_mode == FLIGHTSTATUS_FLIGHTMODE_STABILIZED2    ) mode_str = "st2";	// STABILIZED2
    else if (osd_mode == FLIGHTSTATUS_FLIGHTMODE_STABILIZED3    ) mode_str = "st3";	// STABILIZED3
    else if (osd_mode == FLIGHTSTATUS_FLIGHTMODE_AUTOTUNE       ) mode_str = "at ";	// AUTOTUNE
    else if (osd_mode == FLIGHTSTATUS_FLIGHTMODE_ALTITUDEHOLD   ) mode_str = "ah ";	// ALTITUDEHOLD
    else if (osd_mode == FLIGHTSTATUS_FLIGHTMODE_VELOCITYCONTROL) mode_str = "vc ";	// VELOCITYCONTROL
    else if (osd_mode == FLIGHTSTATUS_FLIGHTMODE_POSITIONHOLD   ) mode_str = "ph ";	// POSITIONHOLD
    osd.printf("%c%s", 0xE0, mode_str);
    osd.closePanel();
}


/******************************************************************/
// Panel  : panBattery A (Voltage 1)
// Needs  : X, Y locations
// Output : Voltage value as in XX.X and symbol of over all battery status
/******************************************************************/
void panBatt_A(int first_col, int first_line){
    osd.setPanel(first_col, first_line);
    osd.openPanel();
    osd.printf("%c%5.2f%c", 0xE2, (double)osd_vbat_A, 0x8E);
    osd.closePanel();
}


/******************************************************************/
// Panel  : panCur_A
// Needs  : X, Y locations
// Output : Current
/******************************************************************/
void panCur_A(int first_col, int first_line){
    osd.setPanel(first_col, first_line);
    osd.openPanel();
    osd.printf("%c%5.2f%c", 0xE4, osd_curr_A * .01, 0x8F);
    osd.closePanel();
}


/******************************************************************/
// Panel  : panBatteryPercent
// Needs  : X, Y locations
// Output : Battery
//          (if defined FLIGHT_BATT_ON_MINIMOSD then not percent but consumed mAh)
/******************************************************************/
void panBatteryPercent(int first_col, int first_line){
    osd.setPanel(first_col, first_line);
    osd.openPanel();
#ifdef FLIGHT_BATT_ON_MINIMOSD
    osd.printf("%6i%c", osd_total_A, 0x82);
#else
    osd.printf("%c%3.0i%c", 0xB9, osd_battery_remaining_A, 0x25);
#endif
    osd.closePanel();
}


/******************************************************************/
// Panel  : panTime
// Needs  : X, Y locations
// Output : Time from bootup or start
/******************************************************************/
void panTime(int first_col, int first_line){
#ifdef JR_SPECIALS	// Time restarts with 00:00 when measured current > 2A for the 1st time
    static unsigned long engine_start_time = 0;
    
    if (engine_start_time == 0 && osd_curr_A > 200) {
        engine_start_time = millis();
    }
    start_Time = (millis() - engine_start_time)/1000;
    
    osd.setPanel(first_col, first_line);
    osd.openPanel();
    osd.printf("%c%2i%c%02i", 0xB3, ((int)start_Time/60)%60, 0x3A, (int)start_Time%60);
    osd.closePanel();
#else
    osd.setPanel(first_col, first_line);
    osd.openPanel();
    start_Time = millis()/1000;
    osd.printf("%c%2i%c%02i", 0xB3, ((int)start_Time/60)%60, 0x3A, (int)start_Time%60);
    osd.closePanel();
#endif
}


/******************************************************************/
// Panel  : panHorizon
// Needs  : X, Y locations
// Output : artificial horizon
/******************************************************************/
void panHorizon(int first_col, int first_line){
    osd.setPanel(first_col, first_line);
    osd.openPanel();
    osd.printf_P(PSTR("\xc8\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\xc9|"));
    osd.printf_P(PSTR("\xc8\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\xc9|"));
    osd.printf_P(PSTR("\xd8\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\xd9|"));
    osd.printf_P(PSTR("\xc8\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\xc9|"));
    osd.printf_P(PSTR("\xc8\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\xc9"));
    osd.closePanel();
    showHorizon((first_col + 1), first_line);
}


#if 0
/******************************************************************/
// Panel  : panOtherUAV
// Needs  : X, Y locations
// Needs  : globals: oUAV_lat, oUAV_lon, oUAV_alt, osd_heading
// Output : shows some mystic info
// Size   : 3 x 6  (rows x chars)
// Status : just an idea and not to forget
//          not compiled
//          not tested
//          not ready
// ToDo   : refactor with function setHomeVars
/******************************************************************/
void panOtherUAV(int first_col, int first_line){
    float dstlon, dstlat;
    long oUAV_distance;
    long oUAV_bearing;
    uint8_t oUAV_direction;
    
    // shrinking factor for longitude going to poles direction
    float rads = fabs(oUAV_lat) * 0.0174532925;
    double scaleLongDown = cos(rads);
    double scaleLongUp   = 1.0f/cos(rads);
   
    // DST to oUAV
    dstlat = fabs(oUAV_lat - osd_lat) * 111319.5;
    dstlon = fabs(oUAV_lon - osd_lon) * 111319.5 * scaleLongDown;
    oUAV_distance = sqrt(sq(dstlat) + sq(dstlon));
    
    // DIR to oUAV
    dstlon = (oUAV_lon - osd_lon);					// OffSet X
    dstlat = (oUAV_lat - osd_lat) * scaleLongUp;			// OffSet Y
    oUAV_bearing = 90 + (atan2(dstlat, -dstlon) * 57.295775);		// absolut oUAV direction
    if (oUAV_bearing < 0) oUAV_bearing += 360;				// normalization
    oUAV_bearing = oUAV_bearing - 180;					// absolut goal direction
    if (oUAV_bearing < 0) oUAV_bearing += 360;				// normalization
    oUAV_bearing = oUAV_bearing - osd_heading;				// relative oUAV direction
    if (oUAV_bearing < 0) oUAV_bearing += 360;				// normalization
    oUAV_direction = round((float)(oUAV_bearing/360.0f) * 16.0f) + 1;	// array of arrows
    if (oUAV_direction > 16) oUAV_direction = 0;
    
    osd.setPanel(first_col, first_line);
    osd.openPanel();
    osd.printf("D%4.0f%c|", (double)((float)(oUAV_distance) * converth), high);
    osd.printf("A%4.0f%c|  ", (double)((oUAV_alt - osd_alt) * converth), high);
    showArrow((uint8_t)oUAV_direction,0);
    osd.closePanel();
}
#endif




// ---------------- EXTRA FUNCTIONS ----------------------

// Show those fancy 2 char arrows
void showArrow(uint8_t rotate_arrow,uint8_t method) {  
    char arrow_set1 = 0x0;
    char arrow_set2 = 0x0;   
    switch (rotate_arrow) {
    case 0: 
        arrow_set1 = 0x90;
        arrow_set2 = 0x91;
        break;
    case 1: 
        arrow_set1 = 0x90;
        arrow_set2 = 0x91;
        break;
    case 2: 
        arrow_set1 = 0x92;
        arrow_set2 = 0x93;
        break;
    case 3: 
        arrow_set1 = 0x94;
        arrow_set2 = 0x95;
        break;
    case 4: 
        arrow_set1 = 0x96;
        arrow_set2 = 0x97;
        break;
    case 5: 
        arrow_set1 = 0x98;
        arrow_set2 = 0x99;
        break;
    case 6: 
        arrow_set1 = 0x9A;
        arrow_set2 = 0x9B;
        break;
    case 7: 
        arrow_set1 = 0x9C;
        arrow_set2 = 0x9D;
        break;
    case 8: 
        arrow_set1 = 0x9E;
        arrow_set2 = 0x9F;
        break;
    case 9: 
        arrow_set1 = 0xA0;
        arrow_set2 = 0xA1;
        break;
    case 10: 
        arrow_set1 = 0xA2;
        arrow_set2 = 0xA3;
        break;
    case 11: 
        arrow_set1 = 0xA4;
        arrow_set2 = 0xA5;
        break;
    case 12: 
        arrow_set1 = 0xA6;
        arrow_set2 = 0xA7;
        break;
    case 13: 
        arrow_set1 = 0xA8;
        arrow_set2 = 0xA9;
        break;
    case 14: 
        arrow_set1 = 0xAA;
        arrow_set2 = 0xAB;
        break;
    case 15: 
        arrow_set1 = 0xAC;
        arrow_set2 = 0xAd;
        break;
    case 16: 
        arrow_set1 = 0xAE;
        arrow_set2 = 0xAF;
        break;
    } 
    if (method == 1) osd.printf("%c%3.0f%c|%c%c", 0xFC, (double)(osd_windspeed * converts), spe, arrow_set1, arrow_set2);
    else osd.printf("%c%c", arrow_set1, arrow_set2);
}


// Calculate and shows artificial horizon
void showHorizon(int start_col, int start_row) { 

    int x, nose, row, minval, hit, subval = 0;
    const int cols = 12;
    const int rows = 5;
    int col_hit[cols];
    float  pitch, roll;

    (abs(osd_pitch) == 90) ? pitch = 89.99 * (90/osd_pitch) * -0.017453293 : pitch = osd_pitch * -0.017453293;
    (abs(osd_roll)  == 90) ? roll =  89.99 * (90/osd_roll)  *  0.017453293 : roll =  osd_roll  *  0.017453293;

    nose = round(tan(pitch) * (rows * 9));
    for (int col=1; col<=cols; col++) {
        x = (col * 12) - (cols * 6) - 6;				//center X point at middle of each col
        col_hit[col-1] = (tan(roll) * x) + nose + (rows*9) - 1;		//calculating hit point on Y plus offset to eliminate negative values
    }

    for (int col=0; col<cols; col++) {
        hit = col_hit[col];
        if (hit > 0 && hit < (rows * 18)) {
            row = rows - ((hit-1) / 18);
            minval = rows * 18 - row * 18 + 1;
            subval = hit - minval;
            subval = round((subval * 9) / 18);
            if (subval == 0) subval = 1;
            printHit(start_col + col, start_row + row - 1, subval);
        }
    }
}


void printHit(byte col, byte row, byte subval) {
    osd.openSingle(col, row);
    char subval_char;
        switch (subval) {
        case 1:
            subval_char = 0x06;
            break;
        case 2:
            subval_char = 0x07; 
            break;
        case 3:
            subval_char = 0x08;
            break;
        case 4:
            subval_char = 0x09;
            break;
        case 5:
            subval_char = 0x0a; 
            break;
        case 6:
            subval_char = 0x0b;
            break;
        case 7:
            subval_char = 0x0c;
            break;
        case 8:
            subval_char = 0x0d;
            break;
        case 9:
            subval_char = 0x0e;
            break;
        }
        osd.printf("%c", subval_char);
}


void set_converts() {
    if (EEPROM.read(measure_ADDR) == 0) {
        converts = 3.6;
        converth = 1.0;
        spe = 0x81;
        high = 0x8D;
    } else {
        converts = 2.23;
        converth = 3.28;
        spe = 0xfb;
        high = 0x66;
    }
}
