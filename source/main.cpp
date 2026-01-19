#define TESLA_INIT_IMPL
#include <tesla.hpp>

#include "debugger.hpp"
#include "dmntcht.h"
#include "memory_dump.hpp"
// #ifdef CUSTOM
#include "Battery.hpp"
#include "audsnoop.h"
#include "Misc.hpp"

#define NVGPU_GPU_IOCTL_PMU_GET_GPU_LOAD 0x80044715
#define FieldDescriptor uint32_t
PadState pad;
// typedef struct {
//     u32 A = 0, B = 0;                                   ///< UniquePadId
// } Breeze_state;
// Breeze_state Bstate = {};

//Common
Thread t0;
Thread t1;
Thread t2;
Thread t3;
Thread t4;
Thread t5;
Thread t6;
Thread t7;
constinit uint64_t systemtickfrequency = 19200000;
bool threadexit = false;
bool threadexit2 = false;
bool Atmosphere_present = false;
uint64_t refreshrate = 1;
FanController g_ICon;

//Misc2
NvChannel nvdecChannel;

//Mini mode
char Variables[672];

//Checks
Result clkrstCheck = 1;
Result nvCheck = 1;
Result pcvCheck = 1;
Result tsCheck = 1;
Result fanCheck = 1;
Result tcCheck = 1;
Result Hinted = 1;
Result pmdmntCheck = 1;
Result dmntchtCheck = 1;
Result psmCheck = 1;
Result audsnoopCheck = 1;
Result nvdecCheck = 1;
Result nifmCheck = 1;

//Wi-Fi
NifmInternetConnectionType NifmConnectionType = (NifmInternetConnectionType)-1;
NifmInternetConnectionStatus NifmConnectionStatus = (NifmInternetConnectionStatus)-1;
bool Nifm_showpass = false;
Result Nifm_internet_rc = -1;
Result Nifm_profile_rc = -1;
NifmNetworkProfileData_new* Nifm_profile = 0;
char Nifm_pass[96];

//NVDEC
uint32_t NVDEC_Hz = 0;
char NVDEC_Hz_c[32];

//DSP
uint32_t DSP_Load_u = -1;
char DSP_Load_c[16];

//Battery
Service *psmService = 0;
BatteryChargeInfoFields *_batteryChargeInfoFields = 0;
char Battery_c[320];

//Temperatures
int32_t SoC_temperaturemiliC = 0;
int32_t PCB_temperaturemiliC = 0;
int32_t skin_temperaturemiliC = 0;
char SoCPCB_temperature_c[64];
char skin_temperature_c[32];

//CPU Usage
uint64_t idletick0 = 19200000;
uint64_t idletick1 = 19200000;
uint64_t idletick2 = 19200000;
uint64_t idletick3 = 19200000;
char CPU_Usage0[32];
char CPU_Usage1[32];
char CPU_Usage2[32];
char CPU_Usage3[32];
char CPU_compressed_c[160];

//Frequency
///CPU
uint32_t CPU_Hz = 0;
char CPU_Hz_c[32];
///GPU
uint32_t GPU_Hz = 0;
char GPU_Hz_c[32];
///RAM
uint32_t RAM_Hz = 0;
char RAM_Hz_c[32];

//RAM Size
char RAM_all_c[64];
char RAM_application_c[64];
char RAM_applet_c[64];
char RAM_system_c[64];
char RAM_systemunsafe_c[64];
char RAM_compressed_c[320];
char RAM_var_compressed_c[320];
uint64_t RAM_Total_all_u = 0;
uint64_t RAM_Total_application_u = 0;
uint64_t RAM_Total_applet_u = 0;
uint64_t RAM_Total_system_u = 0;
uint64_t RAM_Total_systemunsafe_u = 0;
uint64_t RAM_Used_all_u = 0;
uint64_t RAM_Used_application_u = 0;
uint64_t RAM_Used_applet_u = 0;
uint64_t RAM_Used_system_u = 0;
uint64_t RAM_Used_systemunsafe_u = 0;

//Fan
float Rotation_SpeedLevel_f = 0;
char Rotation_SpeedLevel_c[64];

//GPU Usage
FieldDescriptor fd = 0;
uint32_t GPU_Load_u = 0;
char GPU_Load_c[32];

//NX-FPS
bool GameRunning = false;
bool check = false;
bool SaltySD = false;
uintptr_t FPSaddress = 0x0;
uintptr_t FPSavgaddress = 0x0;
uint64_t PID = 0;
uint8_t FPS = 0xFE;
float FPSavg = 254;
char FPS_c[32];
char FPSavg_c[32];
char FPS_compressed_c[64];
char FPS_var_compressed_c[64];
Handle debug;

//Check if SaltyNX is working
bool CheckPort() {
    Handle saltysd;
    for (int i = 0; i < 67; i++) {
        if (R_SUCCEEDED(svcConnectToNamedPort(&saltysd, "InjectServ"))) {
            svcCloseHandle(saltysd);
            break;
        } else {
            if (i == 66) return false;
            svcSleepThread(1'000'000);
        }
    }
    for (int i = 0; i < 67; i++) {
        if (R_SUCCEEDED(svcConnectToNamedPort(&saltysd, "InjectServ"))) {
            svcCloseHandle(saltysd);
            return true;
        } else
            svcSleepThread(1'000'000);
    }
    return false;
}

bool isServiceRunning(const char *serviceName) {
    Handle handle;
    SmServiceName service_name = smEncodeName(serviceName);
    if (R_FAILED(smRegisterService(&handle, service_name, false, 1)))
        return true;
    else {
        svcCloseHandle(handle);
        smUnregisterService(service_name);
        return false;
    }
}

void CheckIfGameRunning(void *) {
    while (threadexit2 == false) {
        if (R_FAILED(pmdmntGetApplicationProcessId(&PID))) {
            if (check == false) {
                remove("sdmc:/SaltySD/FPSoffset.hex");
                check = true;
            }
            GameRunning = false;
            svcCloseHandle(debug);
        } else if (GameRunning == false) {
            svcSleepThread(1'000'000'000);
            FILE *FPSoffset = fopen("sdmc:/SaltySD/FPSoffset.hex", "rb");
            if (FPSoffset != NULL) {
                if (Atmosphere_present == true) {
                    bool out = false;
                    dmntchtHasCheatProcess(&out);
                    if (out == false) dmntchtForceOpenCheatProcess();
                } else
                    svcSleepThread(1'000'000'000);
                fread(&FPSaddress, 0x5, 1, FPSoffset);
                FPSavgaddress = FPSaddress - 0x8;
                fclose(FPSoffset);
                GameRunning = true;
                check = false;
            }
        }
        svcSleepThread(1'000'000'000);
    }
}
// WIP thread
void DoMultiplier();
//Check for input outside of FPS limitations
struct toggle_list_t {
    u32 keycode;
    u32 cheat_id;
};
std::vector<toggle_list_t> m_toggle_list;
bool refresh_cheats = true;
void CheckButtons(void *) {
    padInitializeDefault(&pad);
    // static uint64_t kHeld = padGetButtons(&pad);  // hidKeysHeld(CONTROLLER_P1_AUTO);
    while (threadexit == false) {
        padUpdate(&pad);
        u64 kHeld = padGetButtons(&pad);
        u64 kDown = padGetButtonsDown(&pad);
        for (auto entry : m_toggle_list) {
            if (((kHeld | kDown ) == entry.keycode) && (kDown & entry.keycode)) {
            dmntchtToggleCheat(entry.cheat_id);
            refresh_cheats = true;
            }
        }
        // if ((kHeld & HidNpadButton_ZR) && (kHeld & HidNpadButton_R)) {
        //     if (kHeld & HidNpadButton_Down) {
        //         TeslaFPS = 1;
        //         refreshrate = 1;
        //         systemtickfrequency = 19200000;
        //     } else if (kHeld & HidNpadButton_Up) {
        //         TeslaFPS = 5;
        //         refreshrate = 5;
        //         systemtickfrequency = 3840000;
        //     }
        // }
        // if ((kHeld & HidNpadButton_ZR) && (kDown & HidNpadButton_A)) {
        //     Bstate.A += 2;
        // };
        // if ((kHeld & HidNpadButton_ZR) && (kDown & HidNpadButton_B)) {
        //     Bstate.B += 2;
        // };

        DoMultiplier();

        svcSleepThread(50'000'000); 
    }
}

//Stuff that doesn't need multithreading
void Misc(void *) {
    while (threadexit == false) {
        // CPU, GPU and RAM Frequency
        if (R_SUCCEEDED(clkrstCheck)) {
            ClkrstSession clkSession;
            if (R_SUCCEEDED(clkrstOpenSession(&clkSession, PcvModuleId_CpuBus, 3))) {
                clkrstGetClockRate(&clkSession, &CPU_Hz);
                clkrstCloseSession(&clkSession);
            }
            if (R_SUCCEEDED(clkrstOpenSession(&clkSession, PcvModuleId_GPU, 3))) {
                clkrstGetClockRate(&clkSession, &GPU_Hz);
                clkrstCloseSession(&clkSession);
            }
            if (R_SUCCEEDED(clkrstOpenSession(&clkSession, PcvModuleId_EMC, 3))) {
                clkrstGetClockRate(&clkSession, &RAM_Hz);
                clkrstCloseSession(&clkSession);
            }
        } else if (R_SUCCEEDED(pcvCheck)) {
            pcvGetClockRate(PcvModule_CpuBus, &CPU_Hz);
            pcvGetClockRate(PcvModule_GPU, &GPU_Hz);
            pcvGetClockRate(PcvModule_EMC, &RAM_Hz);
        }

        //Temperatures
        if (R_SUCCEEDED(tsCheck)) {
            tsGetTemperatureMilliC(TsLocation_External, &SoC_temperaturemiliC);
            tsGetTemperatureMilliC(TsLocation_Internal, &PCB_temperaturemiliC);
        }
        if (R_SUCCEEDED(tcCheck)) tcGetSkinTemperatureMilliC(&skin_temperaturemiliC);

        //RAM Memory Used
        if (R_SUCCEEDED(Hinted)) {
            svcGetSystemInfo(&RAM_Total_application_u, 0, INVALID_HANDLE, 0);
            svcGetSystemInfo(&RAM_Total_applet_u, 0, INVALID_HANDLE, 1);
            svcGetSystemInfo(&RAM_Total_system_u, 0, INVALID_HANDLE, 2);
            svcGetSystemInfo(&RAM_Total_systemunsafe_u, 0, INVALID_HANDLE, 3);
            svcGetSystemInfo(&RAM_Used_application_u, 1, INVALID_HANDLE, 0);
            svcGetSystemInfo(&RAM_Used_applet_u, 1, INVALID_HANDLE, 1);
            svcGetSystemInfo(&RAM_Used_system_u, 1, INVALID_HANDLE, 2);
            svcGetSystemInfo(&RAM_Used_systemunsafe_u, 1, INVALID_HANDLE, 3);
        }

        //Fan
        if (R_SUCCEEDED(fanCheck)) fanControllerGetRotationSpeedLevel(&g_ICon, &Rotation_SpeedLevel_f);

        //GPU Load
        if (R_SUCCEEDED(nvCheck)) nvIoctl(fd, NVGPU_GPU_IOCTL_PMU_GET_GPU_LOAD, &GPU_Load_u);

        //FPS
        if (GameRunning == true) {
            if (Atmosphere_present == true) {
                dmntchtReadCheatProcessMemory(FPSaddress, &FPS, 0x1);
                dmntchtReadCheatProcessMemory(FPSavgaddress, &FPSavg, 0x4);
            } else if (R_SUCCEEDED(svcDebugActiveProcess(&debug, PID))) {
                svcReadDebugProcessMemory(&FPS, debug, FPSaddress, 0x1);
                svcReadDebugProcessMemory(&FPSavg, debug, FPSavgaddress, 0x4);
                svcCloseHandle(debug);
            }
        }

        // Interval
        svcSleepThread(1'000'000'000 / refreshrate);
    }
}

void Misc2(void*) {
	while (threadexit == false) {
		//DSP
		if (R_SUCCEEDED(audsnoopCheck)) audsnoopGetDspUsage(&DSP_Load_u);

		//NVDEC clock rate
		if (R_SUCCEEDED(nvdecCheck)) getNvChannelClockRate(&nvdecChannel, 0x75, &NVDEC_Hz);

		if (R_SUCCEEDED(nifmCheck)) {
			u32 dummy = 0;
			Nifm_internet_rc = nifmGetInternetConnectionStatus(&NifmConnectionType, &dummy, &NifmConnectionStatus);
			if (!Nifm_internet_rc && (NifmConnectionType == NifmInternetConnectionType_WiFi))
				Nifm_profile_rc = nifmGetCurrentNetworkProfile((NifmNetworkProfileData*)Nifm_profile);
		}
		// Interval
		svcSleepThread(100'000'000);
	}
}

//Check each core for idled ticks in intervals, they cannot read info about other core than they are assigned
void CheckCore0(void *) {
    while (threadexit == false) {
        static uint64_t idletick_a0 = 0;
        static uint64_t idletick_b0 = 0;
        svcGetInfo(&idletick_b0, InfoType_IdleTickCount, INVALID_HANDLE, 0);
        svcSleepThread(1'000'000'000 / refreshrate);
        svcGetInfo(&idletick_a0, InfoType_IdleTickCount, INVALID_HANDLE, 0);
        idletick0 = idletick_a0 - idletick_b0;
    }
}

void CheckCore1(void *) {
    while (threadexit == false) {
        static uint64_t idletick_a1 = 0;
        static uint64_t idletick_b1 = 0;
        svcGetInfo(&idletick_b1, InfoType_IdleTickCount, INVALID_HANDLE, 1);
        svcSleepThread(1'000'000'000 / refreshrate);
        svcGetInfo(&idletick_a1, InfoType_IdleTickCount, INVALID_HANDLE, 1);
        idletick1 = idletick_a1 - idletick_b1;
    }
}

void CheckCore2(void *) {
    while (threadexit == false) {
        static uint64_t idletick_a2 = 0;
        static uint64_t idletick_b2 = 0;
        svcGetInfo(&idletick_b2, InfoType_IdleTickCount, INVALID_HANDLE, 2);
        svcSleepThread(1'000'000'000 / refreshrate);
        svcGetInfo(&idletick_a2, InfoType_IdleTickCount, INVALID_HANDLE, 2);
        idletick2 = idletick_a2 - idletick_b2;
    }
}

void CheckCore3(void *) {
    while (threadexit == false) {
        static uint64_t idletick_a3 = 0;
        static uint64_t idletick_b3 = 0;
        svcGetInfo(&idletick_b3, InfoType_IdleTickCount, INVALID_HANDLE, 3);
        svcSleepThread(1'000'000'000 / refreshrate);
        svcGetInfo(&idletick_a3, InfoType_IdleTickCount, INVALID_HANDLE, 3);
        idletick3 = idletick_a3 - idletick_b3;
    }
}

//Start reading all stats
void StartThreads() {
    // threadCreate(&t0, CheckCore0, NULL, NULL, 0x100, 0x10, 0);
    // threadCreate(&t1, CheckCore1, NULL, NULL, 0x100, 0x10, 1);
    // threadCreate(&t2, CheckCore2, NULL, NULL, 0x100, 0x10, 2);
    // threadCreate(&t3, CheckCore3, NULL, NULL, 0x100, 0x10, 3);
    // threadCreate(&t4, Misc, NULL, NULL, 0x100, 0x3F, -2);
    threadCreate(&t5, CheckButtons, NULL, NULL, 0x400, 0x3F, -2);
    // threadStart(&t0);
    // threadStart(&t1);
    // threadStart(&t2);
    // threadStart(&t3);
    // threadStart(&t4);
    threadStart(&t5);
}

//End reading all stats
void CloseThreads() {
    threadexit = true;
    threadexit2 = true;
    // threadWaitForExit(&t0);
    // threadWaitForExit(&t1);
    // threadWaitForExit(&t2);
    // threadWaitForExit(&t3);
    // threadWaitForExit(&t4);
    threadWaitForExit(&t5);
    // threadWaitForExit(&t6);
    // threadWaitForExit(&t7);
    // threadClose(&t0);
    // threadClose(&t1);
    // threadClose(&t2);
    // threadClose(&t3);
    // threadClose(&t4);
    threadClose(&t5);
    // threadClose(&t6);
    // threadClose(&t7);
    threadexit = false;
    threadexit2 = false;
}

//Separate functions dedicated to "FPS Counter" mode
void FPSCounter(void *) {
    while (threadexit == false) {
        if (GameRunning == true) {
            if (Atmosphere_present == true)
                dmntchtReadCheatProcessMemory(FPSavgaddress, &FPSavg, 0x4);
            else if (R_SUCCEEDED(svcDebugActiveProcess(&debug, PID))) {
                svcReadDebugProcessMemory(&FPSavg, debug, FPSavgaddress, 0x4);
                svcCloseHandle(debug);
            }
        } else
            FPSavg = 254;
        //interval
        svcSleepThread(1'000'000'000 / refreshrate);
    }
}

void StartFPSCounterThread() {
    threadCreate(&t0, FPSCounter, NULL, NULL, 0x1000, 0x3F, 3);
    threadStart(&t0);
}

void EndFPSCounterThread() {
    threadexit = true;
    threadWaitForExit(&t0);
    threadClose(&t0);
    threadexit = false;
}

//FPS Counter mode
class com_FPS : public tsl::Gui {
   public:
    com_FPS() {}

    virtual tsl::elm::Element *createUI() override {
        auto rootFrame = new tsl::elm::OverlayFrame("", "");

        auto Status = new tsl::elm::CustomDrawer([](tsl::gfx::Renderer *renderer, u16 x, u16 y, u16 w, u16 h) {
            static uint8_t avg = 0;
            if (FPSavg < 10) avg = 0;
            if (FPSavg >= 10) avg = 23;
            if (FPSavg >= 100) avg = 46;
            renderer->drawRect(0, 0, tsl::cfg::FramebufferWidth - 370 + avg, 50, a(0x7111));
            renderer->drawString(FPSavg_c, false, 5, 40, 40, renderer->a(0xFFFF));
        });

        rootFrame->setContent(Status);

        return rootFrame;
    }

    virtual void update() override {
        ///FPS
        snprintf(FPSavg_c, sizeof FPSavg_c, "%2.1f", FPSavg);
    }
    virtual bool handleInput(u64 keysDown, u64 keysHeld, HidTouchState touchInput, JoystickPosition leftJoyStick, JoystickPosition rightJoyStick) override {
        if ((keysHeld & HidNpadButton_StickL) && (keysHeld & HidNpadButton_StickR)) {
            EndFPSCounterThread();
            tsl::goBack();
            return true;
        }
        return false;
    }
};

//Full mode
class FullOverlay : public tsl::Gui {
   public:
    FullOverlay() {}

    virtual tsl::elm::Element *createUI() override {
        auto rootFrame = new tsl::elm::OverlayFrame("Full mode", APP_VERSION);

        auto Status = new tsl::elm::CustomDrawer([](tsl::gfx::Renderer *renderer, u16 x, u16 y, u16 w, u16 h) {
            //Print strings
            ///CPU
            if (R_SUCCEEDED(clkrstCheck) || R_SUCCEEDED(pcvCheck)) {
                renderer->drawString("CPU Usage:", false, 20, 120, 20, renderer->a(0xFFFF));
                renderer->drawString(CPU_Hz_c, false, 20, 155, 15, renderer->a(0xFFFF));
                renderer->drawString(CPU_compressed_c, false, 20, 185, 15, renderer->a(0xFFFF));
            }

            ///GPU
            if (R_SUCCEEDED(clkrstCheck) || R_SUCCEEDED(pcvCheck) || R_SUCCEEDED(nvCheck)) {
                renderer->drawString("GPU Usage:", false, 20, 285, 20, renderer->a(0xFFFF));
                if (R_SUCCEEDED(clkrstCheck) || R_SUCCEEDED(pcvCheck)) renderer->drawString(GPU_Hz_c, false, 20, 320, 15, renderer->a(0xFFFF));
                if (R_SUCCEEDED(nvCheck)) renderer->drawString(GPU_Load_c, false, 20, 335, 15, renderer->a(0xFFFF));
            }

            ///RAM
            if (R_SUCCEEDED(clkrstCheck) || R_SUCCEEDED(pcvCheck) || R_SUCCEEDED(Hinted)) {
                renderer->drawString("RAM Usage:", false, 20, 375, 20, renderer->a(0xFFFF));
                if (R_SUCCEEDED(clkrstCheck) || R_SUCCEEDED(pcvCheck)) renderer->drawString(RAM_Hz_c, false, 20, 410, 15, renderer->a(0xFFFF));
                if (R_SUCCEEDED(Hinted)) {
                    renderer->drawString(RAM_compressed_c, false, 20, 440, 15, renderer->a(0xFFFF));
                    renderer->drawString(RAM_var_compressed_c, false, 140, 440, 15, renderer->a(0xFFFF));
                }
            }

            ///Thermal
            if (R_SUCCEEDED(tsCheck) || R_SUCCEEDED(tcCheck) || R_SUCCEEDED(fanCheck)) {
                renderer->drawString("Thermal:", false, 20, 540, 20, renderer->a(0xFFFF));
                if (R_SUCCEEDED(tsCheck)) renderer->drawString(SoCPCB_temperature_c, false, 20, 575, 15, renderer->a(0xFFFF));
                if (R_SUCCEEDED(tcCheck)) renderer->drawString(skin_temperature_c, false, 20, 605, 15, renderer->a(0xFFFF));
                if (R_SUCCEEDED(fanCheck)) renderer->drawString(Rotation_SpeedLevel_c, false, 20, 620, 15, renderer->a(0xFFFF));
            }

            ///FPS
            if (GameRunning == true) {
                renderer->drawString(FPS_compressed_c, false, 235, 120, 20, renderer->a(0xFFFF));
                renderer->drawString(FPS_var_compressed_c, false, 295, 120, 20, renderer->a(0xFFFF));
            }

            if (refreshrate == 5)
                renderer->drawString("Hold Left Stick & Right Stick to Exit\nHold ZR + R + D-Pad Down to slow down refresh", false, 20, 675, 15, renderer->a(0xFFFF));
            else if (refreshrate == 1)
                renderer->drawString("Hold Left Stick & Right Stick to Exit\nHold ZR + R + D-Pad Up to speed up refresh", false, 20, 675, 15, renderer->a(0xFFFF));
        });

        rootFrame->setContent(Status);

        return rootFrame;
    }

    virtual void update() override {
        if (TeslaFPS == 60) TeslaFPS = 1;
        //In case of getting more than systemtickfrequency in idle, make it equal to systemtickfrequency to get 0% as output and nothing less
        //This is because making each loop also takes time, which is not considered because this will take also additional time
        if (idletick0 > systemtickfrequency) idletick0 = systemtickfrequency;
        if (idletick1 > systemtickfrequency) idletick1 = systemtickfrequency;
        if (idletick2 > systemtickfrequency) idletick2 = systemtickfrequency;
        if (idletick3 > systemtickfrequency) idletick3 = systemtickfrequency;

        //Make stuff ready to print
        ///CPU
        snprintf(CPU_Hz_c, sizeof CPU_Hz_c, "Frequency: %.1f MHz", (float)CPU_Hz / 1000000);
        snprintf(CPU_Usage0, sizeof CPU_Usage0, "Core #0: %.2f%s", ((double)systemtickfrequency - (double)idletick0) / (double)systemtickfrequency * 100, "%");
        snprintf(CPU_Usage1, sizeof CPU_Usage1, "Core #1: %.2f%s", ((double)systemtickfrequency - (double)idletick1) / (double)systemtickfrequency * 100, "%");
        snprintf(CPU_Usage2, sizeof CPU_Usage2, "Core #2: %.2f%s", ((double)systemtickfrequency - (double)idletick2) / (double)systemtickfrequency * 100, "%");
        snprintf(CPU_Usage3, sizeof CPU_Usage3, "Core #3: %.2f%s", ((double)systemtickfrequency - (double)idletick3) / (double)systemtickfrequency * 100, "%");
        snprintf(CPU_compressed_c, sizeof CPU_compressed_c, "%s\n%s\n%s\n%s", CPU_Usage0, CPU_Usage1, CPU_Usage2, CPU_Usage3);

        ///GPU
        snprintf(GPU_Hz_c, sizeof GPU_Hz_c, "Frequency: %.1f MHz", (float)GPU_Hz / 1000000);
        snprintf(GPU_Load_c, sizeof GPU_Load_c, "Load: %.1f%s", (float)GPU_Load_u / 10, "%");

        ///RAM
        snprintf(RAM_Hz_c, sizeof RAM_Hz_c, "Frequency: %.1f MHz", (float)RAM_Hz / 1000000);
        float RAM_Total_application_f = (float)RAM_Total_application_u / 1024 / 1024;
        float RAM_Total_applet_f = (float)RAM_Total_applet_u / 1024 / 1024;
        float RAM_Total_system_f = (float)RAM_Total_system_u / 1024 / 1024;
        float RAM_Total_systemunsafe_f = (float)RAM_Total_systemunsafe_u / 1024 / 1024;
        float RAM_Total_all_f = RAM_Total_application_f + RAM_Total_applet_f + RAM_Total_system_f + RAM_Total_systemunsafe_f;
        float RAM_Used_application_f = (float)RAM_Used_application_u / 1024 / 1024;
        float RAM_Used_applet_f = (float)RAM_Used_applet_u / 1024 / 1024;
        float RAM_Used_system_f = (float)RAM_Used_system_u / 1024 / 1024;
        float RAM_Used_systemunsafe_f = (float)RAM_Used_systemunsafe_u / 1024 / 1024;
        float RAM_Used_all_f = RAM_Used_application_f + RAM_Used_applet_f + RAM_Used_system_f + RAM_Used_systemunsafe_f;
        snprintf(RAM_all_c, sizeof RAM_all_c, "Total:");
        snprintf(RAM_application_c, sizeof RAM_application_c, "Application:");
        snprintf(RAM_applet_c, sizeof RAM_applet_c, "Applet:");
        snprintf(RAM_system_c, sizeof RAM_system_c, "System:");
        snprintf(RAM_systemunsafe_c, sizeof RAM_systemunsafe_c, "System Unsafe:");
        snprintf(RAM_compressed_c, sizeof RAM_compressed_c, "%s\n%s\n%s\n%s\n%s", RAM_all_c, RAM_application_c, RAM_applet_c, RAM_system_c, RAM_systemunsafe_c);
        snprintf(RAM_all_c, sizeof RAM_all_c, "%4.2f / %4.2f MB", RAM_Used_all_f, RAM_Total_all_f);
        snprintf(RAM_application_c, sizeof RAM_application_c, "%4.2f / %4.2f MB", RAM_Used_application_f, RAM_Total_application_f);
        snprintf(RAM_applet_c, sizeof RAM_applet_c, "%4.2f / %4.2f MB", RAM_Used_applet_f, RAM_Total_applet_f);
        snprintf(RAM_system_c, sizeof RAM_system_c, "%4.2f / %4.2f MB", RAM_Used_system_f, RAM_Total_system_f);
        snprintf(RAM_systemunsafe_c, sizeof RAM_systemunsafe_c, "%4.2f / %4.2f MB", RAM_Used_systemunsafe_f, RAM_Total_systemunsafe_f);
        snprintf(RAM_var_compressed_c, sizeof RAM_var_compressed_c, "%s\n%s\n%s\n%s\n%s", RAM_all_c, RAM_application_c, RAM_applet_c, RAM_system_c, RAM_systemunsafe_c);

        ///Thermal
        snprintf(SoCPCB_temperature_c, sizeof SoCPCB_temperature_c, "SoC: %2.2f \u00B0C\nPCB: %2.2f \u00B0C", (float)SoC_temperaturemiliC / 1000, (float)PCB_temperaturemiliC / 1000);
        snprintf(skin_temperature_c, sizeof skin_temperature_c, "Skin: %2.2f \u00B0C", (float)skin_temperaturemiliC / 1000);
        snprintf(Rotation_SpeedLevel_c, sizeof Rotation_SpeedLevel_c, "Fan: %2.2f%s", Rotation_SpeedLevel_f * 100, "%");

        ///FPS
        snprintf(FPS_c, sizeof FPS_c, "PFPS:");       //Pushed Frames Per Second
        snprintf(FPSavg_c, sizeof FPSavg_c, "FPS:");  //Frames Per Second calculated from averaged frametime
        snprintf(FPS_compressed_c, sizeof FPS_compressed_c, "%s\n%s", FPS_c, FPSavg_c);
        snprintf(FPS_var_compressed_c, sizeof FPS_var_compressed_c, "%u\n%2.2f", FPS, FPSavg);
    }
    virtual bool handleInput(u64 keysDown, u64 keysHeld, HidTouchState touchInput, JoystickPosition leftJoyStick, JoystickPosition rightJoyStick) override {
        if ((keysHeld & HidNpadButton_StickL) && (keysHeld & HidNpadButton_StickR)) {
            CloseThreads();
            tsl::goBack();
            return true;
        }
        return false;
    }
};
// Bookmark display
// WIP
bool b_showkeyhint = true;
bool b_showallcheats = true;
tsl::elm::ToggleListItem *ShowALlCheats, *ShowKeyHints;
std::string Title_str = "";
#define MAX_POINTER_DEPTH 12
struct pointer_chain_t {
    u64 depth = 0;
    s64 offset[MAX_POINTER_DEPTH + 1] = {0};  // offset to address pointed by pointer
};
struct bookmark_t {
    char label[19] = {0};
    searchType_t type;
    pointer_chain_t pointer;
    bool heap = true;
    u64 offset = 0;
    bool deleted = false;
    u8 multiplier = 1;
	u16 magic = 0x1289;
};
typedef struct {
    bool is_outline = false;
    bool always_expanded = false;
    bool expanded = false;
    uint32_t index = 0, size = 0;  // index into m_cheats
} cheat_outline_entry_t;
std::vector<cheat_outline_entry_t> m_cheat_outline;
bool m_outline_mode = true;
bool m_outline_refresh = true;
bool show_outline_off = false;
#define NUM_bookmark 10
#define MAX_NUM_cheats 35
u32 m_NUM_cheats = 20;
#define NUM_cheats m_NUM_cheats
#define NUM_combokey 3
u32 total_opcode = 0;
#define MaxCheatCount 0x80
#define MaxOpcodes 0x100 // uint32_t opcodes[0x100]
#define MaximumProgramOpcodeCount 0x400
u8 fontsize = 15;
bool m_editCheat = false;
u8 keycount;
Result rc;
std::string m_titleName = "", m_titleName2 = "", m_versionString = "";
char m_cheatcode_path[128];
char m_toggle_path[128];
u64 m_cheatCnt = 0; 
DmntCheatEntry *m_cheats = nullptr;
struct outline_t {
    std::string label;
    u32 index;  // m_cheatlist_offset
};
// Cache
struct cache_outline_t {
    std::string label;
    bool selected = false;
    bool is_outline = false;
    bool expanded = false;
    bool always_expanded = false;
    u32 size = 0;
    u32 index;  // m_cheatlist_offset
};
std::vector<cache_outline_t> m_cache_outline;
std::vector<DmntCheatEntry> m_cache;
// end Cache
u32 m_outline_index = 0;
bool m_show_outline = false;
std::vector<outline_t> m_outline;  // WIP
bool save_code_to_file = false;
bool m_edit_value = false;
bool m_hex_mode = false;
bool m_get_toggle_keycode = false;
bool m_get_action_keycode = false;
bool save_breeze_toggle_to_file = false;
bool save_breeze_action_to_file = false;
bool first_launch = true;
static const std::vector<std::string> keyNames = {"0","1","2","3","4","5","6","7","8","9","A","B","C","D","E","F","-","."};
static const std::vector<std::string> actionNames = {"+","*","Set","Freeze","Unfreeze"};
typedef enum {
    Add,
    Multipy,
    Set,
    Freeze,
    Unfreeze
} breeze_action_t;
struct breeze_action_list_t {
    u32 keycode = 0;
    breeze_action_t breeze_action = Add;
    searchValue_t value = {0}, freeze_value = {0};
    u8 index = 0;
};
std::vector<breeze_action_list_t> m_breeze_action_list;
std::string valueStr = "";
u8 value_pos =0;
u8 m_value_edit_index = 0;
u64 m_selected_address;
searchType_t m_selected_type;
typedef enum {
    Display,
    Insert,
    Delete
} valueStr_action_t;
std::string valueStr_edit_display(valueStr_action_t action) {
    std::string tempstr = valueStr;
    switch (action) {
        case Display:
            return tempstr.insert(value_pos,"|");
        case Insert:
            return valueStr.insert(value_pos,keyNames[m_value_edit_index]);
        case Delete:
            if ((valueStr.length() > 0) && (value_pos <= valueStr.length()) && (value_pos > 0))
                return valueStr.erase(value_pos - 1, 1);
            return valueStr;
    }
    return "";
}
static const std::vector<u32> buttonCodes = {0x80000001,
                                             0x80000002,
                                             0x80000004,
                                             0x80000008,
                                             0x80000010,
                                             0x80000020,
                                             0x80000040,
                                             0x80000080,
                                             0x80000100,
                                             0x80000200,
                                             0x80000400,
                                             0x80000800,
                                             0x80001000,
                                             0x80002000,
                                             0x80004000,
                                             0x80008000,
                                             0x80010000,
                                             0x80020000,
                                             0x80040000,
                                             0x80080000,
                                             0x80100000,
                                             0x80200000,
                                             0x80400000,
                                             0x80800000};
static const std::vector<std::string> buttonNames = {"\uE0A0 ", "\uE0A1 ", "\uE0A2 ", "\uE0A3 ", "\uE0C4 ", "\uE0C5 ", "\uE0A4 ", "\uE0A5 ", "\uE0A6 ", "\uE0A7 ", "\uE0B3 ", "\uE0B4 ", "\uE0B1 ", "\uE0AF ", "\uE0B2 ", "\uE0B0 ", "\uE091 ", "\uE092 ", "\uE090 ", "\uE093 ", "\uE145 ", "\uE143 ", "\uE146 ", "\uE144 "};

char BookmarkLabels[NUM_bookmark * 20 + MAX_NUM_cheats * 0x41] = "";
char Cursor[NUM_bookmark * 5 + MAX_NUM_cheats * 5 + 1000] = "";
char MultiplierStr[NUM_bookmark * 5 + MAX_NUM_cheats * 0x41 ] = "";
char CheatsLabelsStr[MAX_NUM_cheats * 0x41 ] = "";
char CheatsCursor[MAX_NUM_cheats * 5 +500 ] = "";
char CheatsEnableStr[MAX_NUM_cheats * 0x41] = "";
// char m_err_str[500] = "";
#define m_err_str CheatsLabelsStr
bool m_show_only_enabled_cheats = true;
bool m_cursor_on_bookmark = true;
bool m_no_cheats = true;
bool m_no_bookmarks = true;
bool m_game_not_running = true;
bool m_on_show = false;
u32 m_displayed_bookmark_lines = 0;
u32 m_displayed_cheat_lines = 0;
u32 m_index = 0;
u32 m_cheat_index = 0, m_cheat_index_save = 0;
std::string m_edizon_dir = "/switch/EdiZon";
std::string m_store_extension = "A";
Debugger *m_debugger;
// MemoryDump *m_memoryDump;
MemoryDump *m_AttributeDumpBookmark = nullptr;
u8 m_addresslist_offset = 0;
u32 m_cheatlist_offset = 0, m_cheatlist_offset_save = 0;
bool m_32bitmode = false;
static const std::vector<u8> dataTypeSizes = {1, 1, 2, 2, 4, 4, 8, 8, 4, 8, 8};
searchValue_t m_oldvalue[NUM_bookmark] = {0};
DmntCheatProcessMetadata metadata;
// char Variables[NUM_bookmark*20];
char bookmarkfilename[200] = "bookmark filename";
u8 build_id[0x20];
bool init_se_tools() {
    if (dmntchtCheck == 1) dmntchtCheck = dmntchtInitialize();

    m_debugger = new Debugger();
    dmntchtForceOpenCheatProcess();
    dmntchtHasCheatProcess(&(m_debugger->m_dmnt));
    if (m_debugger->m_dmnt) {
        dmntchtGetCheatProcessMetadata(&metadata);
            size_t appControlDataSize = 0;
            static NsApplicationControlData m_appControlData = {};
            NacpLanguageEntry *languageEntry = nullptr;
            std::memset(&m_appControlData, 0x00, sizeof(NsApplicationControlData));
            rc = nsGetApplicationControlData(NsApplicationControlSource_Storage, metadata.title_id & 0xFFFFFFFFFFFFFFF0, &m_appControlData, sizeof(NsApplicationControlData), &appControlDataSize);
            if (rc == 0) {
                rc = nsGetApplicationDesiredLanguage(&m_appControlData.nacp, &languageEntry);
                if (languageEntry != nullptr) {
                    m_titleName = std::string(languageEntry->name);
                    // m_titleName2 = std::string(languageEntry[1].name);
                };
                m_versionString = std::string(m_appControlData.nacp.display_version);
            };
    }
    else {
        return false;
        LoaderModuleInfo proc_modules[2] = {};
        s32 num_modules = 2;
        ldrDmntInitialize();
        Result rc = ldrDmntGetProcessModuleInfo(m_debugger->getRunningApplicationPID(), proc_modules, std::size(proc_modules), &num_modules);
        ldrDmntExit();
        const LoaderModuleInfo *proc_module = nullptr;
        if (num_modules == 2) {
            proc_module = &proc_modules[1];
        } else if (num_modules == 1) {
            proc_module = &proc_modules[0];
        }
        if (rc != 0)
            printf("num_modules = %x, proc_module->base_address = %lx , pid = %ld, rc = %x\n ", num_modules, proc_module->base_address, m_debugger->getRunningApplicationPID(), rc);
        metadata.main_nso_extents.base = proc_module->base_address;
        metadata.main_nso_extents.size = proc_module->size;
        Handle proc_h;        
        {
            typedef struct {
                u64 keys_held;
                u64 flags;
            } CfgOverrideStatus;
            struct {
                NcmProgramLocation loc;
                CfgOverrideStatus status;
            } out;
            u64 pid = m_debugger->getRunningApplicationPID();
            serviceDispatchInOut(pmdmntGetServiceSession(), 65000, pid, out,
                                             .out_handle_attrs = {SfOutHandleAttr_HipcCopy},
                                             .out_handles = &proc_h, );
        };
        rc = svcGetInfo(&metadata.heap_extents.base, InfoType_HeapRegionAddress, proc_h, 0);
        // printf("metadata.heap_extents.base = %lx rc = %x\n", metadata.heap_extents.base, rc);
        std::memcpy(metadata.main_nso_build_id, proc_module->build_id, sizeof((metadata.main_nso_build_id)));
    };
    // dmntchtGetCheatProcessMetadata(&metadata);
    memcpy(build_id, metadata.main_nso_build_id, 0x20);

    // if (!(m_debugger->m_dmnt)) {m_debugger->attachToProcess();m_debugger->resume();}
    // check and set m_32bitmode

    snprintf(bookmarkfilename, 200, "%s/%02x%02x%02x%02x%02x%02x%02x%02x.dat", EDIZON_DIR,
             build_id[0], build_id[1], build_id[2], build_id[3], build_id[4], build_id[5], build_id[6], build_id[7]);

    m_AttributeDumpBookmark = new MemoryDump(bookmarkfilename, DumpType::ADDR, false);
    // m_memoryDump = new MemoryDump(EDIZON_DIR "/memdumpbookmark.dat", DumpType::ADDR, false);
    return true;
};
void cleanup_se_tools() {
    // if (dmntchtCheck == 0) dmntchtExit();
    delete m_debugger;
    delete m_AttributeDumpBookmark;
    // delete m_memoryDump;
    return;
};
void dumpcodetofile() {
    FILE *pfile;
    char tmp[1000];
    snprintf(m_cheatcode_path, 128, "sdmc:/atmosphere/contents/%016lX/cheats/%02X%02X%02X%02X%02X%02X%02X%02X.txt", metadata.title_id, build_id[0], build_id[1], build_id[2], build_id[3], build_id[4], build_id[5], build_id[6], build_id[7]);
    snprintf(m_toggle_path, 128, "sdmc:/atmosphere/contents/%016lX/cheats/toggles.txt", metadata.title_id);
    pfile = fopen(m_cheatcode_path, "w");
    if (pfile != NULL) {
        snprintf(tmp, 1000, "[Zing %s %s %s TID: %016lX BID: %02X%02X%02X%02X%02X%02X%02X%02X]\n\n", APP_VERSION, m_titleName.c_str(), m_versionString.c_str(), metadata.title_id,
                 build_id[0], build_id[1], build_id[2], build_id[3], build_id[4], build_id[5], build_id[6], build_id[7]);
        fputs(tmp, pfile);
        for (u32 i = 0; i < m_cheatCnt; i++) {
            // output outlines
            if (m_outline.size() > 1) {
                tmp[0] = 0;
                for (auto entry : m_outline) {
                    if (entry.index > i) break;
                    if (entry.index == i) snprintf(tmp, sizeof tmp, "[%s]\n", entry.label.c_str());
                }
                if (strlen(tmp) > 0) fputs(tmp, pfile);
            }

            if ((i == 0) && (m_cheats[0].cheat_id == 0))
                snprintf(tmp, 1000, "{%s}\n", m_cheats[i].definition.readable_name);
            else
                snprintf(tmp, 1000, "[%s]\n", m_cheats[i].definition.readable_name);
            fputs(tmp, pfile);
            for (u32 j = 0; j < m_cheats[i].definition.num_opcodes; j++) {
                u16 opcode = (m_cheats[i].definition.opcodes[j] >> 28) & 0xF;
                u8 T = (m_cheats[i].definition.opcodes[j] >> 24) & 0xF;
                if ((opcode == 9) && (((m_cheats[i].definition.opcodes[j] >> 8) & 0xF) == 0)) {
                    snprintf(tmp, 1000, "%08X\n", m_cheats[i].definition.opcodes[j]);
                    fputs(tmp, pfile);
                    continue;
                }
                if (opcode == 0xC) {
                    opcode = (m_cheats[i].definition.opcodes[j] >> 24) & 0xFF;
                    T = (m_cheats[i].definition.opcodes[j] >> 20) & 0xF;
                    u8 X = (m_cheats[i].definition.opcodes[j] >> 8) & 0xF;
                    if (opcode == 0xC0) {
                        opcode = opcode * 16 + X;
                    }
                }
                if (opcode == 10) {
                    u8 O = (m_cheats[i].definition.opcodes[j] >> 8) & 0xF;
                    if (O == 2 || O == 4 || O == 5)
                        T = 8;
                    else
                        T = 4;
                }
                switch (opcode) {
                    case 0:
                    case 1:
                        snprintf(tmp, sizeof(tmp), "%08X ", m_cheats[i].definition.opcodes[j++]);
                        fputs(tmp, pfile);
                        // 3+1
                    case 9:
                    case 0xC04:
                        // 2+1
                        snprintf(tmp, sizeof(tmp), "%08X ", m_cheats[i].definition.opcodes[j++]);
                        fputs(tmp, pfile);
                    case 3:
                    case 10:
                        // 1+1
                        snprintf(tmp, sizeof(tmp), "%08X ", m_cheats[i].definition.opcodes[j]);
                        fputs(tmp, pfile);
                        if (T == 8 || (T == 0 && opcode == 3)) {
                            j++;
                            snprintf(tmp, sizeof(tmp), "%08X ", m_cheats[i].definition.opcodes[j]);
                            fputs(tmp, pfile);
                        }
                        break;
                    case 4:
                    case 6:
                        // 3
                        snprintf(tmp, sizeof(tmp), "%08X ", m_cheats[i].definition.opcodes[j++]);
                        fputs(tmp, pfile);
                    case 5:
                    case 7:
                    case 0xC00:
                    case 0xC02:
                        snprintf(tmp, sizeof(tmp), "%08X ", m_cheats[i].definition.opcodes[j++]);
                        fputs(tmp, pfile);
                        // 2
                    case 2:
                    case 8:
                    case 0xC1:
                    case 0xC2:
                    case 0xC3:
                    case 0xC01:
                    case 0xC03:
                    case 0xC05:
                    default:
                        snprintf(tmp, sizeof(tmp), "%08X ", m_cheats[i].definition.opcodes[j]);
                        fputs(tmp, pfile);
                        // 1
                        break;
                }
                if (j >= (m_cheats[i].definition.num_opcodes))  // better to be ugly than to corrupt
                {
                    printf("error encountered in addcodetofile \n ");
                    for (u32 k = 0; k < m_cheats[i].definition.num_opcodes; k++) {
                        snprintf(tmp, sizeof(tmp), "%08X ", m_cheats[i].definition.opcodes[k++]);
                        fputs(tmp, pfile);
                    }
                    snprintf(tmp, sizeof(tmp), "\n");
                    fputs(tmp, pfile);
                    break;
                }
                snprintf(tmp, sizeof(tmp), "\n");
                fputs(tmp, pfile);
            }
            snprintf(tmp, sizeof(tmp), "\n");
            fputs(tmp, pfile);
        }
        fclose(pfile);
    }
}
void save_breeze_toggle(){
    std::string breeze_toggle_filename = bookmarkfilename;
    breeze_toggle_filename.replace((breeze_toggle_filename.length()-3),3,"bz1");
    MemoryDump *m_breeze_toggle_file;
    m_breeze_toggle_file = new MemoryDump(breeze_toggle_filename.c_str(), DumpType::ADDR, true);
    for (auto entry : m_toggle_list) {
        m_breeze_toggle_file->addData((u8 *)&entry, sizeof(toggle_list_t));
    }
    delete m_breeze_toggle_file;
}
void load_breeze_toggle(){
    m_toggle_list.clear();
    std::string breeze_toggle_filename = bookmarkfilename;
    breeze_toggle_filename.replace((breeze_toggle_filename.length()-3),3,"bz1");
    MemoryDump *m_breeze_toggle_file;
    toggle_list_t entry;
    m_breeze_toggle_file = new MemoryDump(breeze_toggle_filename.c_str(), DumpType::ADDR, false);
    if (m_breeze_toggle_file->size() > 0)
        for (size_t i = 0; i < m_breeze_toggle_file->size() / sizeof(toggle_list_t); i++) {
            m_breeze_toggle_file->getData(i * sizeof(toggle_list_t), &entry, sizeof(toggle_list_t));
            m_toggle_list.push_back(entry);
        }
    delete m_breeze_toggle_file;
}
void save_breeze_action(){
    std::string breeze_action_filename = bookmarkfilename;
    breeze_action_filename.replace((breeze_action_filename.length()-3),3,"bz2");
    MemoryDump *m_breeze_acton_file;
    m_breeze_acton_file = new MemoryDump(breeze_action_filename.c_str(), DumpType::ADDR, true);
    for (auto entry : m_breeze_action_list) {
        m_breeze_acton_file->addData((u8 *)&entry, sizeof(breeze_action_list_t));
    }
    delete m_breeze_acton_file;
}
void load_breeze_action(){
    m_breeze_action_list.clear();
    std::string breeze_action_filename = bookmarkfilename;
    breeze_action_filename.replace((breeze_action_filename.length()-3),3,"bz2");
    MemoryDump *m_breeze_acton_file;
    breeze_action_list_t entry;
    m_breeze_acton_file = new MemoryDump(breeze_action_filename.c_str(), DumpType::ADDR, false);
    if (m_breeze_acton_file->size() > 0)
        for (size_t i = 0; i < m_breeze_acton_file->size() / sizeof(breeze_action_list_t); i++) {
            m_breeze_acton_file->getData(i * sizeof(breeze_action_list_t), &entry, sizeof(breeze_action_list_t));
            m_breeze_action_list.push_back(entry);
        }
    delete m_breeze_acton_file;
}
// WIP loadcache
bool loadcachefromfile() {
    m_cache_outline.clear();
    m_cache.clear();
    u32 _index = 0;
    bool copy_cheat_file = false;
    // 
    snprintf(m_cheatcode_path, 128, "sdmc:/switch/zing/%02X%02X%02X%02X%02X%02X%02X%02X.txt", build_id[0], build_id[1], build_id[2], build_id[3], build_id[4], build_id[5], build_id[6], build_id[7]);
    // if (access(m_cheatcode_path, F_OK) != 0)
    FILE *pfile, *pfile2 = NULL;
    pfile = fopen(m_cheatcode_path, "rb");
    if (pfile == NULL) {
        char atm_path[128];
        snprintf(atm_path, 128, "sdmc:/atmosphere/contents/%016lX/cheats/%02X%02X%02X%02X%02X%02X%02X%02X.txt", metadata.title_id, build_id[0], build_id[1], build_id[2], build_id[3], build_id[4], build_id[5], build_id[6], build_id[7]);
        pfile = fopen(atm_path, "rb");
        copy_cheat_file = true;
    }
    if (pfile != NULL) {
        fseek(pfile, 0, SEEK_END);
        size_t len = ftell(pfile);
        u8 *s = new u8[len];
        fseek(pfile, 0, SEEK_SET);
        fread(s, 1, len, pfile);
        fclose(pfile); 
        if (copy_cheat_file) {
            pfile2 = fopen(m_cheatcode_path, "wb");
            if (pfile2 != NULL) {
                fwrite(s, 1, len, pfile2);
                fclose(pfile2);
            }
        }
        DmntCheatEntry cheatentry;
        cheatentry.definition.num_opcodes = 0;
        cheatentry.enabled = false;
        u8 label_len = 0;
        size_t i = 0;
        cache_outline_t entry = {};
        while (i < len) {
            if (std::isspace(static_cast<unsigned char>(s[i]))) {
                /* Just ignore whitespace. */
                i++;
            } else if (s[i] == '[') {
                if (cheatentry.definition.num_opcodes != 0) {
                    if (cheatentry.definition.opcodes[0] == 0x20000000) {
                        if (entry.is_outline) {
                            entry.size = _index - entry.index -1;
                            m_cache_outline.push_back(entry);
                        }
                        entry.index = _index;
                        entry.expanded = false;
                        entry.always_expanded = false;
                        entry.label = cheatentry.definition.readable_name;
                        entry.is_outline = true;
                    }
                    // if (cheatentry.enabled == true)
                    //     // dmntchtSetMasterCheat(&(cheatentry.definition));
                    // else
                        m_cache.push_back(cheatentry);
                        // dmntchtAddCheat(&(cheatentry.definition), cheatentry.enabled, &(cheatentry.cheat_id));
                    _index++;
                } 
                // else 
                // {
                //     outline_t entry;
                //     entry.index = _index;
                //     entry.label = cheatentry.definition.readable_name;
                //     if (label_len > 0) {
                //         if (last_entry == _index) m_cache_outline.pop_back();
                //         m_cache_outline.push_back(entry);
                //         last_entry = _index;
                //     }
                // }
                /* Parse a normal cheat set to off */
                cheatentry.definition.num_opcodes = 0;
                cheatentry.enabled = false;
                /* Extract name bounds. */
                size_t j = i + 1;
                while (s[j] != ']') {
                    j++;
                    if (j >= len) {
                        return false;
                    }
                }
                /* s[i+1:j] is cheat name. */
                const size_t cheat_name_len = std::min(j - i - 1, sizeof(cheatentry.definition.readable_name));
                std::memcpy(cheatentry.definition.readable_name, &s[i + 1], cheat_name_len);
                for (u32 i = 0; i < cheat_name_len; i++) {
                    if (cheatentry.definition.readable_name[i] == 13 || cheatentry.definition.readable_name[i] == 10) cheatentry.definition.readable_name[i] = 32;
                };
                cheatentry.definition.readable_name[cheat_name_len] = 0;
                label_len = cheat_name_len;

                /* Skip onwards. */
                i = j + 1;
            } else if (s[i] == '(') {
                size_t j = i + 1;
                while (s[j] != ')') {
                    j++;
                    if (j >= len) {
                        return false;
                    }
                }
                i = j + 1;
            } else if (s[i] == '{') {
                if (cheatentry.definition.num_opcodes != 0) {
                    m_cache.push_back(cheatentry);
                    _index++;
                    // dmntchtAddCheat(&(cheatentry.definition), cheatentry.enabled, &(cheatentry.cheat_id));
                }
                /* We're parsing a master cheat. Turn it on */
                cheatentry.definition.num_opcodes = 0;
                cheatentry.enabled = true;
                /* Extract name bounds */
                size_t j = i + 1;
                while (s[j] != '}') {
                    j++;
                    if (j >= len) {
                        return false;
                    }
                }

                /* s[i+1:j] is cheat name. */
                const size_t cheat_name_len = std::min(j - i - 1, sizeof(cheatentry.definition.readable_name));
                memcpy(cheatentry.definition.readable_name, &s[i + 1], cheat_name_len);
                cheatentry.definition.readable_name[cheat_name_len] = 0;
                label_len = cheat_name_len;
                // strcpy(cheatentry.definition.readable_name, "master code");

                /* Skip onwards. */
                i = j + 1;
            } else if (std::isxdigit(static_cast<unsigned char>(s[i]))) {
                if (label_len == 0)
                    return false;
                /* Bounds check the opcode count. */
                if (cheatentry.definition.num_opcodes >= sizeof(cheatentry.definition.opcodes) / 4) {
                    if (cheatentry.definition.num_opcodes != 0) {
                        m_cache.push_back(cheatentry);
                        // dmntchtAddCheat(&(cheatentry.definition), cheatentry.enabled, &(cheatentry.cheat_id));
                    }
                    return false;
                }

                /* We're parsing an instruction, so validate it's 8 hex digits. */
                for (size_t j = 1; j < 8; j++) {
                    /* Validate 8 hex chars. */
                    if (i + j >= len || !std::isxdigit(static_cast<unsigned char>(s[i + j]))) {
                        if (cheatentry.definition.num_opcodes != 0) {
                            m_cache.push_back(cheatentry);
                            // dmntchtAddCheat(&(cheatentry.definition), cheatentry.enabled, &(cheatentry.cheat_id));
                        }
                        return false;
                    }
                }

                /* Parse the new opcode. */
                char hex_str[9] = {0};
                std::memcpy(hex_str, &s[i], 8);
                cheatentry.definition.opcodes[cheatentry.definition.num_opcodes++] = std::strtoul(hex_str, NULL, 16);

                /* Skip onwards. */
                i += 8;
            } else {
                /* Unexpected character encountered. */
                if (cheatentry.definition.num_opcodes != 0) {
                    m_cache.push_back(cheatentry);
                    // dmntchtAddCheat(&(cheatentry.definition), cheatentry.enabled, &(cheatentry.cheat_id));
                }
                return false;
            }
        }
        if (cheatentry.definition.num_opcodes != 0) {
            m_cache.push_back(cheatentry);
            // dmntchtAddCheat(&(cheatentry.definition), cheatentry.enabled, &(cheatentry.cheat_id));
        }
        if (entry.is_outline) {
            entry.size = _index - entry.index - 1;
            m_cache_outline.push_back(entry);
        }
        return true;
    }
    {
        cache_outline_t entry = {};
        DmntCheatEntry centry = {};
        strcat(m_cheatcode_path, " fail to load");
        strcpy((centry.definition.readable_name), m_cheatcode_path);
        m_cache.push_back(centry);
        m_cache_outline.push_back(entry);
    }
    return false;
}
// WIP loadcheats
bool loadcheatsfromfile() {
    m_outline.clear();
    u8 _index = 0;
    u8 last_entry = 0xFF;
    snprintf(m_cheatcode_path, 128, "sdmc:/atmosphere/contents/%016lX/cheats/%02X%02X%02X%02X%02X%02X%02X%02X.txt", metadata.title_id, build_id[0], build_id[1], build_id[2], build_id[3], build_id[4], build_id[5], build_id[6], build_id[7]);
    
    FILE *pfile;
    pfile = fopen(m_cheatcode_path, "rb");
    if (pfile == NULL) {
        char zing_path[128];
        snprintf(zing_path, 128, "sdmc:/switch/zing/%02X%02X%02X%02X%02X%02X%02X%02X.txt", build_id[0], build_id[1], build_id[2], build_id[3], build_id[4], build_id[5], build_id[6], build_id[7]);
        pfile = fopen(zing_path, "rb");
    }
    if (pfile != NULL) {
        fseek(pfile, 0, SEEK_END);
        size_t len = ftell(pfile);
        u8 *s = new u8[len];
        fseek(pfile, 0, SEEK_SET);
        fread(s, 1, len, pfile);
        fclose(pfile);
        DmntCheatEntry cheatentry;
        cheatentry.definition.num_opcodes = 0;
        cheatentry.enabled = false;
        u8 label_len = 0;
        size_t i = 0;
        while (i < len) {
            if (std::isspace(static_cast<unsigned char>(s[i]))) {
                /* Just ignore whitespace. */
                i++;
            } else if (s[i] == '[') {
                if (cheatentry.definition.num_opcodes != 0) {
                    if (cheatentry.definition.opcodes[0] == 0x20000000) {
                        outline_t entry;
                        entry.index = _index;
                        entry.label = cheatentry.definition.readable_name;
                        if (label_len > 0) {
                            if (last_entry == _index) m_outline.pop_back();
                            m_outline.push_back(entry);
                            last_entry = _index;
                        }
                    }
                    if (cheatentry.enabled == true)
                        dmntchtSetMasterCheat(&(cheatentry.definition));
                    else
                        dmntchtAddCheat(&(cheatentry.definition), cheatentry.enabled, &(cheatentry.cheat_id));
                    _index++;
                } 
                // else 
                // {
                //     outline_t entry;
                //     entry.index = _index;
                //     entry.label = cheatentry.definition.readable_name;
                //     if (label_len > 0) {
                //         if (last_entry == _index) m_outline.pop_back();
                //         m_outline.push_back(entry);
                //         last_entry = _index;
                //     }
                // }
                /* Parse a normal cheat set to off */
                cheatentry.definition.num_opcodes = 0;
                cheatentry.enabled = false;
                /* Extract name bounds. */
                size_t j = i + 1;
                while (s[j] != ']') {
                    j++;
                    if (j >= len) {
                        return false;
                    }
                }
                /* s[i+1:j] is cheat name. */
                const size_t cheat_name_len = std::min(j - i - 1, sizeof(cheatentry.definition.readable_name));
                std::memcpy(cheatentry.definition.readable_name, &s[i + 1], cheat_name_len);
                for (u32 i = 0; i < cheat_name_len; i++) {
                    if (cheatentry.definition.readable_name[i] == 13 || cheatentry.definition.readable_name[i] == 10) cheatentry.definition.readable_name[i] = 32;
                };
                cheatentry.definition.readable_name[cheat_name_len] = 0;
                label_len = cheat_name_len;

                /* Skip onwards. */
                i = j + 1;
            } else if (s[i] == '(') {
                size_t j = i + 1;
                while (s[j] != ')') {
                    j++;
                    if (j >= len) {
                        return false;
                    }
                }
                i = j + 1;
            } else if (s[i] == '{') {
                if (cheatentry.definition.num_opcodes != 0) {
                    dmntchtAddCheat(&(cheatentry.definition), cheatentry.enabled, &(cheatentry.cheat_id));
                }
                /* We're parsing a master cheat. Turn it on */
                cheatentry.definition.num_opcodes = 0;
                cheatentry.enabled = true;
                /* Extract name bounds */
                size_t j = i + 1;
                while (s[j] != '}') {
                    j++;
                    if (j >= len) {
                        return false;
                    }
                }

                /* s[i+1:j] is cheat name. */
                const size_t cheat_name_len = std::min(j - i - 1, sizeof(cheatentry.definition.readable_name));
                memcpy(cheatentry.definition.readable_name, &s[i + 1], cheat_name_len);
                cheatentry.definition.readable_name[cheat_name_len] = 0;
                label_len = cheat_name_len;
                // strcpy(cheatentry.definition.readable_name, "master code");

                /* Skip onwards. */
                i = j + 1;
            } else if (std::isxdigit(static_cast<unsigned char>(s[i]))) {
                if (label_len == 0)
                    return false;
                /* Bounds check the opcode count. */
                if (cheatentry.definition.num_opcodes >= sizeof(cheatentry.definition.opcodes) / 4) {
                    if (cheatentry.definition.num_opcodes != 0) {
                        dmntchtAddCheat(&(cheatentry.definition), cheatentry.enabled, &(cheatentry.cheat_id));
                    }
                    return false;
                }

                /* We're parsing an instruction, so validate it's 8 hex digits. */
                for (size_t j = 1; j < 8; j++) {
                    /* Validate 8 hex chars. */
                    if (i + j >= len || !std::isxdigit(static_cast<unsigned char>(s[i + j]))) {
                        if (cheatentry.definition.num_opcodes != 0) {
                            dmntchtAddCheat(&(cheatentry.definition), cheatentry.enabled, &(cheatentry.cheat_id));
                        }
                        return false;
                    }
                }

                /* Parse the new opcode. */
                char hex_str[9] = {0};
                std::memcpy(hex_str, &s[i], 8);
                cheatentry.definition.opcodes[cheatentry.definition.num_opcodes++] = std::strtoul(hex_str, NULL, 16);

                /* Skip onwards. */
                i += 8;
            } else {
                /* Unexpected character encountered. */
                if (cheatentry.definition.num_opcodes != 0) {
                    dmntchtAddCheat(&(cheatentry.definition), cheatentry.enabled, &(cheatentry.cheat_id));
                }
                return false;
            }
        }
        if (cheatentry.definition.num_opcodes != 0) {
            dmntchtAddCheat(&(cheatentry.definition), cheatentry.enabled, &(cheatentry.cheat_id));
        }
        return true;
    }
    return false;
}
DmntCheatEntry *GetCheatEntryByReadableName(const char *readable_name) {
    /* Check all non-master cheats for match. */
    for (size_t i = 0; i < m_cheatCnt; i++) {
        if (std::strncmp(m_cheats[i].definition.readable_name, readable_name, sizeof(m_cheats[i].definition.readable_name)) == 0) {
            return &m_cheats[i];
        }
    }
    return nullptr;
}
bool ParseCheatToggles(const char *s, size_t len) {
    size_t i = 0;
    char cur_cheat_name[sizeof(DmntCheatDefinition::readable_name)];
    char toggle[8];
    while (i < len) {
        if (std::isspace(static_cast<unsigned char>(s[i]))) {
            /* Just ignore whitespace. */
            i++;
        } else if (s[i] == '[') {
            /* Extract name bounds. */
            size_t j = i + 1;
            while (s[j] != ']') {
                j++;
                if (j >= len) {
                    return false;
                }
            }
            /* s[i+1:j] is cheat name. */
            const size_t cheat_name_len = std::min(j - i - 1, sizeof(cur_cheat_name));
            std::memcpy(cur_cheat_name, &s[i + 1], cheat_name_len);
            cur_cheat_name[cheat_name_len] = 0;
            /* Skip onwards. */
            i = j + 1;
            /* Skip whitespace. */
            while (std::isspace(static_cast<unsigned char>(s[i]))) {
                i++;
            }
            /* Parse whether to toggle. */
            j = i + 1;
            while (!std::isspace(static_cast<unsigned char>(s[j]))) {
                j++;
                if (j >= len || (j - i) >= sizeof(toggle)) {
                    return false;
                }
            }
            /* s[i:j] is toggle. */
            const size_t toggle_len = (j - i);
            std::memcpy(toggle, &s[i], toggle_len);
            toggle[toggle_len] = 0;
            /* Allow specifying toggle for not present cheat. */
            DmntCheatEntry *entry = GetCheatEntryByReadableName(cur_cheat_name);
            if (entry != nullptr) {
                if (strcasecmp(toggle, "1") == 0 || strcasecmp(toggle, "true") == 0 || strcasecmp(toggle, "on") == 0) {
                    if (entry->enabled != true) dmntchtToggleCheat(entry->cheat_id);
                    entry->enabled = true;
                } else if (strcasecmp(toggle, "0") == 0 || strcasecmp(toggle, "false") == 0 || strcasecmp(toggle, "off") == 0) {
                    if (entry->enabled != false) dmntchtToggleCheat(entry->cheat_id);
                    entry->enabled = false;
                }
            }
            /* Skip onwards. */
            i = j + 1;
        } else {
            /* Unexpected character encountered. */
            return false;
        }
    }
    return true;
}
void loadtoggles() {
    snprintf(m_toggle_path, 128, "sdmc:/atmosphere/contents/%016lX/cheats/toggles.brz", metadata.title_id);
    FILE *pfile;
    pfile = fopen(m_toggle_path, "rb");
    if (pfile != NULL) {
        fseek(pfile, 0, SEEK_END);
        size_t len = ftell(pfile);
        char *s = new char[len];
        fseek(pfile, 0, SEEK_SET);
        fread(s, 1, len, pfile);
        ParseCheatToggles(s, len);
        fclose(pfile);
    };
};
void savetoggles() {
    snprintf(m_toggle_path, 128, "sdmc:/atmosphere/contents/%016lX/cheats/toggles.brz", metadata.title_id);
    FILE *pfile;
    char tmp[1000];
    pfile = fopen(m_toggle_path, "w");
    if (pfile != NULL) {
        for (u8 i = 0; i < m_cheatCnt; i++) {
            snprintf(tmp, 1000, "[%s]\n%s\n", m_cheats[i].definition.readable_name, (m_cheats[i].enabled) ? "true" : "false");
            fputs(tmp, pfile);
        }
        fclose(pfile);
    };
};
void DoMultiplier() {
    for (u8 line = 0; line < NUM_bookmark; line++) {
        if ((line + m_addresslist_offset) >= (m_AttributeDumpBookmark->size() / sizeof(bookmark_t)))
            break;
        bookmark_t bookmark;
        {
            u64 address = 0;
            m_AttributeDumpBookmark->getData((line + m_addresslist_offset) * sizeof(bookmark_t), &bookmark, sizeof(bookmark_t));
            if (bookmark.magic != 0x1289) bookmark.multiplier = 1;
            if (bookmark.multiplier==1) continue;
            if (bookmark.pointer.depth > 0)  // check if pointer chain point to valid address update address if necessary
            {
                // bool updateaddress = true;
                u64 nextaddress = metadata.main_nso_extents.base;  //m_mainBaseAddr;
                for (int z = bookmark.pointer.depth; z >= 0; z--) {
                    nextaddress += bookmark.pointer.offset[z];
                    MemoryInfo meminfo = m_debugger->queryMemory(nextaddress);
                    if (meminfo.perm == Perm_Rw)
                        if (z == 0) {
                            if (address == nextaddress) {
                                // updateaddress = false;
                            } else {
                                address = nextaddress;
                            }
                        } else
                            m_debugger->readMemory(&nextaddress, ((m_32bitmode) ? sizeof(u32) : sizeof(u64)), nextaddress);
                    else {
                        // updateaddress = false;
                        break;
                    }
                }
            } else {
                address = ((bookmark.heap) ? ((m_debugger->queryMemory(metadata.heap_extents.base).type == 0) ? metadata.alias_extents.base
                                                                                                              : metadata.heap_extents.base)
                                           : metadata.main_nso_extents.base) +
                          bookmark.offset;
            };
            searchValue_t value = {0};
            m_debugger->readMemory(&value, dataTypeSizes[bookmark.type], address);
            if ((m_oldvalue[line]._u64 == 0) || (m_oldvalue[line]._s64 > value._s64))
                m_oldvalue[line]._s64 = value._s64;
            else if (bookmark.multiplier != 1) {
                switch (bookmark.type) {
                    case SEARCH_TYPE_FLOAT_32BIT:
                        if (m_oldvalue[line]._f32 < value._f32) {
                            m_oldvalue[line]._f32 = (value._f32 - m_oldvalue[line]._f32) * bookmark.multiplier + m_oldvalue[line]._f32;
                        };
                        break;
                    case SEARCH_TYPE_FLOAT_64BIT:
                        if (m_oldvalue[line]._f64 < value._f64) {
                            m_oldvalue[line]._f64 = (value._f64 - m_oldvalue[line]._f64) * bookmark.multiplier + m_oldvalue[line]._f64;
                        };
                        break;
                    case SEARCH_TYPE_UNSIGNED_8BIT:
                    case SEARCH_TYPE_UNSIGNED_16BIT:
                    case SEARCH_TYPE_UNSIGNED_32BIT:
                    case SEARCH_TYPE_UNSIGNED_64BIT:
                        if (m_oldvalue[line]._u64 < value._u64) {
                            m_oldvalue[line]._u64 = (value._u64 - m_oldvalue[line]._u64) * bookmark.multiplier + m_oldvalue[line]._u64;
                        };
                        break;
                    default:
                        if (m_oldvalue[line]._s64 < value._s64) {
                            m_oldvalue[line]._s64 = (value._s64 - m_oldvalue[line]._s64) * bookmark.multiplier + m_oldvalue[line]._s64;
                        };
                        break;
                };
                m_debugger->writeMemory(&(m_oldvalue[line]), dataTypeSizes[bookmark.type], address);
            };
        }
    }
}
static std::string _getAddressDisplayString(u64 address, Debugger *debugger, searchType_t searchType) {
    char ss[200];

    searchValue_t searchValue;
    searchValue._u64 = debugger->peekMemory(address);
    // start mod for address content display
    //   u16 k = searchValue._u8;
    //   if (m_searchValueFormat == FORMAT_HEX)
    //   {
    //     switch (dataTypeSizes[searchType])
    //     {
    //     case 1:
    //       ss << "0x" << std::uppercase << std::hex << k;
    //       break;
    //     case 2:
    //       ss << "0x" << std::uppercase << std::hex << searchValue._u16;
    //       break;
    //     default:
    //     case 4:
    //       ss << "0x" << std::uppercase << std::hex << searchValue._u32;
    //       break;
    //     case 8:
    //       ss << "0x" << std::uppercase << std::hex << searchValue._u64;
    //       break;
    //     }
    //   }
    //   else
    {
        // end mod
        switch (searchType) {
            case SEARCH_TYPE_UNSIGNED_8BIT:
                snprintf(ss, sizeof ss, "%d", searchValue._u8);
                // ss << std::dec << static_cast<u64>(searchValue._u8);
                break;
            case SEARCH_TYPE_UNSIGNED_16BIT:
                snprintf(ss, sizeof ss, "%d", searchValue._u16);
                //   ss << std::dec << static_cast<u64>(searchValue._u16);
                break;
            case SEARCH_TYPE_UNSIGNED_32BIT:
                snprintf(ss, sizeof ss, "%d", searchValue._u32);
                //   ss << std::dec << static_cast<u64>(searchValue._u32);
                break;
            case SEARCH_TYPE_UNSIGNED_64BIT:
                snprintf(ss, sizeof ss, "0x%016lX", searchValue._u64);
                //   ss << std::dec << static_cast<u64>(searchValue._u64);
                break;
            case SEARCH_TYPE_SIGNED_8BIT:
                snprintf(ss, sizeof ss, "%d", searchValue._s8);
                //   ss << std::dec << static_cast<s64>(searchValue._s8);
                break;
            case SEARCH_TYPE_SIGNED_16BIT:
                snprintf(ss, sizeof ss, "%d", searchValue._s16);
                //   ss << std::dec << static_cast<s64>(searchValue._s16);
                break;
            case SEARCH_TYPE_SIGNED_32BIT:
                snprintf(ss, sizeof ss, "%d", searchValue._s32);
                //   ss << std::dec << static_cast<s64>(searchValue._s32);
                break;
            case SEARCH_TYPE_SIGNED_64BIT:
                snprintf(ss, sizeof ss, "%ld", searchValue._s64);
                //   ss << std::dec << static_cast<s64>(searchValue._s64);
                break;
            case SEARCH_TYPE_FLOAT_32BIT:
                snprintf(ss, sizeof ss, "%.2f", searchValue._f32);
                //   ss << std::dec << searchValue._f32;
                break;
            case SEARCH_TYPE_FLOAT_64BIT:
                snprintf(ss, sizeof ss, "%.2lf", searchValue._f64);
                //   ss << std::dec << searchValue._f64;
                break;
            case SEARCH_TYPE_POINTER:
                snprintf(ss, sizeof ss, "0x%010lX", searchValue._u64);
                //   ss << std::dec << searchValue._u64;
                break;
            case SEARCH_TYPE_NONE:
                break;
        }
    }

    return ss;  //.str();
}
bool deletebookmark(){
    std::string old_bookmarkfilename = bookmarkfilename;
    bookmark_t bookmark;
    old_bookmarkfilename.replace((old_bookmarkfilename.length()-3),3,"old");
    MemoryDump *m_old_AttributeDumpBookmark;
    m_old_AttributeDumpBookmark = new MemoryDump(old_bookmarkfilename.c_str(), DumpType::ADDR, true);
    for (size_t i = 0; i < m_AttributeDumpBookmark->size() / sizeof(bookmark_t); i++) {
        {
            m_AttributeDumpBookmark->getData(i * sizeof(bookmark_t), &bookmark, sizeof(bookmark_t));
            m_old_AttributeDumpBookmark->addData((u8 *)&bookmark, sizeof(bookmark_t));
        }
    }
    delete m_AttributeDumpBookmark;
    m_AttributeDumpBookmark = new MemoryDump(bookmarkfilename, DumpType::ADDR, true);
    for (size_t i = 0; i < m_old_AttributeDumpBookmark->size() / sizeof(bookmark_t); i++) {
        if (i != m_index + m_addresslist_offset) {
            m_old_AttributeDumpBookmark->getData(i * sizeof(bookmark_t), &bookmark, sizeof(bookmark_t));
            m_AttributeDumpBookmark->addData((u8 *)&bookmark, sizeof(bookmark_t));
        }
    }
    delete m_old_AttributeDumpBookmark;
    return true;
}
bool addbookmark() {
    // printf("start adding cheat to bookmark\n");
    // m_cheatCnt
    u32 index = m_cheat_index + m_cheatlist_offset;
    if (m_outline_mode) {
        index = m_cheat_outline[index].index;
    };
    DmntCheatDefinition cheat = m_cheats[index].definition;
    bookmark_t bookmark;
    memcpy(&bookmark.label, &cheat.readable_name, sizeof(bookmark.label));
    if ((bookmark.label[18] & 0xC0) == 0xC0) {
        bookmark.label[18] = 0;
    } else if ((bookmark.label[17] & 0xE0) == 0xE0) {
        bookmark.label[17] = 0;
        bookmark.label[18] = 0;
    } else if ((bookmark.label[16] & 0xF0) == 0xF0) {
        bookmark.label[16] = 0;
        bookmark.label[17] = 0;
        bookmark.label[18] = 0;
    }
    bookmark.pointer.depth = 0;
    bookmark.deleted = false;
    bool success = false;
    u64 offset[MAX_POINTER_DEPTH + 1] = {0};
    u64 depth = 0;
    // u64 address;
    bool no7 = true;

    for (u8 i = 0; i < cheat.num_opcodes; i++) {
        u8 opcode = (cheat.opcodes[i] >> 28) & 0xF;
        // u8 Register = (cheat.opcodes[i] >> 16) & 0xF;
        u8 FSA = (cheat.opcodes[i] >> 12) & 0xF;
        u8 T = (cheat.opcodes[i] >> 24) & 0xF;
        u8 M = (cheat.opcodes[i] >> 20) & 0xF;
        u8 A = cheat.opcodes[i] & 0xFF;

        // printf("code %x opcode %d register %d FSA %d %x \n", cheat.opcodes[i], opcode, Register, FSA, cheat.opcodes[i + 1]);

        if (depth > MAX_POINTER_DEPTH) {
            strncat(m_err_str, "this code is bigger than space catered on the bookmark !!\n", sizeof m_err_str -1);
            break;
        }

        if (opcode == 0) {  //static case
            i++;
            bookmark.offset = cheat.opcodes[i] + A * 0x100000000;
            switch (T) {
                case 1:
                    bookmark.type = SEARCH_TYPE_UNSIGNED_8BIT;
                    i++;
                    break;
                case 2:
                    bookmark.type = SEARCH_TYPE_UNSIGNED_16BIT;
                    i++;
                    break;
                case 4:
                    bookmark.type = SEARCH_TYPE_UNSIGNED_32BIT;
                    i++;
                    break;
                case 8:
                    bookmark.type = SEARCH_TYPE_UNSIGNED_64BIT;
                    i += 2;
                    break;
                default:
                    strncat(m_err_str, "cheat code processing error, wrong width value\n", sizeof m_err_str -1);
                    bookmark.type = SEARCH_TYPE_UNSIGNED_32BIT;
                    i++;
                    break;
            };
            if (M != 0) {
                bookmark.heap = true;
                // address = (m_debugger->queryMemory(metadata.heap_extents.base).type == 0) ? metadata.alias_extents.base : metadata.heap_extents.base + bookmark.offset;
            } else {
                bookmark.heap = false;
                // address = metadata.main_nso_extents.base + bookmark.offset;
            }

            m_AttributeDumpBookmark->addData((u8 *)&bookmark, sizeof(bookmark_t));
            break;
        }
        if (depth == 0) {
            if (opcode == 5 && FSA == 0) {
                i++;
                if (M == 0)
                    offset[depth] = cheat.opcodes[i];
                else
                    offset[depth] = (m_debugger->queryMemory(metadata.heap_extents.base).type == 0) ? metadata.alias_extents.base : metadata.heap_extents.base - metadata.main_nso_extents.base + cheat.opcodes[i];
                depth++;
            }
            continue;
        }
        if (opcode == 5 && FSA == 1) {
            i++;
            offset[depth] = cheat.opcodes[i];
            depth++;
            continue;
        }
        if (opcode == 7 && FSA == 0) {
            i++;
            offset[depth] = cheat.opcodes[i];
            // success = true;
            no7 = false;
            continue;
            // break;
        }
        if (opcode == 6) {
            if (no7) {
                offset[depth] = 0;
            }
            switch (T) {
                case 1:
                    bookmark.type = SEARCH_TYPE_UNSIGNED_8BIT;
                    break;
                case 2:
                    bookmark.type = SEARCH_TYPE_UNSIGNED_16BIT;
                    break;
                case 4:
                    bookmark.type = SEARCH_TYPE_UNSIGNED_32BIT;
                    if (((cheat.opcodes[i + 2] & 0xF0000000) == 0x40000000) || ((cheat.opcodes[i + 2] & 0xF0000000) == 0x30000000) || ((cheat.opcodes[i + 2] & 0xF0000000) == 0xC0000000))
                        bookmark.type = SEARCH_TYPE_FLOAT_32BIT;
                    break;
                case 8:
                    bookmark.type = SEARCH_TYPE_UNSIGNED_64BIT;
                    if (((cheat.opcodes[i + 1] & 0xF0000000) == 0x40000000) || ((cheat.opcodes[i + 1] & 0xF0000000) == 0x30000000) || ((cheat.opcodes[i + 1] & 0xF0000000) == 0xC0000000))
                        bookmark.type = SEARCH_TYPE_FLOAT_64BIT;
                    break;
                default:
                    strncat(m_err_str, "cheat code processing error, wrong width value\n", sizeof m_err_str -1);
                    bookmark.type = SEARCH_TYPE_UNSIGNED_32BIT;
                    break;
            }
            success = true;
            break;
        }
    }

    if (success) {
        // compute address
        // printf("success ! \n");
        bookmark.pointer.depth = depth;
        u64 nextaddress = metadata.main_nso_extents.base;
        // printf("main[%lx]", nextaddress);
        u8 i = 0;
        for (int z = depth; z >= 0; z--) {
            // bookmark_t bm;
            bookmark.pointer.offset[z] = offset[i];
            // printf("+%lx z=%d ", bookmark.pointer.offset[z], z);
            nextaddress += bookmark.pointer.offset[z];
            // printf("[%lx]", nextaddress);
            // m_memoryDumpBookmark->addData((u8 *)&nextaddress, sizeof(u64));
            // m_AttributeDumpBookmark->addData((u8 *)&bm, sizeof(bookmark_t));
            MemoryInfo meminfo = m_debugger->queryMemory(nextaddress);
            if (meminfo.perm == Perm_Rw) {
                // address = nextaddress;
                if (m_32bitmode)
                    m_debugger->readMemory(&nextaddress, sizeof(u32), nextaddress);
                else
                    m_debugger->readMemory(&nextaddress, sizeof(u64), nextaddress);
            } else {
                // printf("*access denied*\n");
                success = false;
                // break;
            }
            // printf("(%lx)", nextaddress);
            i++;
        }
        // printf("\n");
    }
    if (success) {
        // m_memoryDumpBookmark->addData((u8 *)&address, sizeof(u64));
        m_AttributeDumpBookmark->addData((u8 *)&bookmark, sizeof(bookmark_t));
        strncat(m_err_str, "Adding pointer chain from cheat to bookmark\n", sizeof m_err_str -1);
    } 
    else {
        if (bookmark.pointer.depth > 2)  // depth of 2 means only one pointer hit high chance of wrong positive
            //     {
            //         (new MessageBox("Extracted pointer chain is broken on current memory state\n \n If the game is in correct state\n \n would you like to try to rebase the chain?", MessageBox::YES_NO))
            //             ->setSelectionAction([&](u8 selection) {
            //                 if (selection) {
            //                     searchValue_t value;
            //                     while (!getinput("Enter the value at this memory", "You must know what type is the value and set it correctly in the search memory type setting", "", &value)) {
            //                     }
            //                     printf("value = %ld\n", value._u64);
            //                     rebasepointer(value);  //bookmark);
            //                 } else {
            //                     // add broken pointer chain for reference
            // m_memoryDumpBookmark->addData((u8 *)&address, sizeof(u64));
            m_AttributeDumpBookmark->addData((u8 *)&bookmark, sizeof(bookmark_t));
        //                 }
        //                 Gui::g_currMessageBox->hide();
        //             })
        //             ->show();
        //     } else
        //         (new Snackbar("Not able to extract pointer chain from cheat"))->show();
    }

    // pointercheck(); //disable for now;
    // m_AttributeDumpBookmark->flushBuffer();
    return true;
}
class MailBoxOverlay : public tsl::Gui {
   public:
    MailBoxOverlay() {}

    virtual tsl::elm::Element *createUI() override {
        auto rootFrame = new tsl::elm::OverlayFrame("", "");

        auto Status = new tsl::elm::CustomDrawer([](tsl::gfx::Renderer *renderer, u16 x, u16 y, u16 w, u16 h) {
            if (GameRunning == false)
                renderer->drawRect(0, 0, tsl::cfg::FramebufferWidth - 150, 80, a(0x7111));
            else
                renderer->drawRect(0, 0, tsl::cfg::FramebufferWidth - 150, 110, a(0x7111));

            //Print strings
            ///CPU
            if (GameRunning == true)
                renderer->drawString("CPU\nGPU\nRAM\nTEMP\nFAN\nPFPS\nFPS", false, 0, 15, 15, renderer->a(0xFFFF));
            else
                renderer->drawString("A\nB\nTeslaFPS\nTEMP\nFAN", false, 0, 15, 15, renderer->a(0xFFFF));

            ///GPU
            renderer->drawString(Variables, false, 60, 15, 15, renderer->a(0xFFFF));
        });

        rootFrame->setContent(Status);

        return rootFrame;
    }

    virtual void update() override {
        if (TeslaFPS == 60) TeslaFPS = 1;
        //In case of getting more than systemtickfrequency in idle, make it equal to systemtickfrequency to get 0% as output and nothing less
        //This is because making each loop also takes time, which is not considered because this will take also additional time
        if (idletick0 > systemtickfrequency) idletick0 = systemtickfrequency;
        if (idletick1 > systemtickfrequency) idletick1 = systemtickfrequency;
        if (idletick2 > systemtickfrequency) idletick2 = systemtickfrequency;
        if (idletick3 > systemtickfrequency) idletick3 = systemtickfrequency;

        //Make stuff ready to print
        ///CPU
        double percent = ((double)systemtickfrequency - (double)idletick0) / (double)systemtickfrequency * 100;
        snprintf(CPU_Usage0, sizeof CPU_Usage0, "%.0f%s", percent, "%");
        percent = ((double)systemtickfrequency - (double)idletick1) / (double)systemtickfrequency * 100;
        snprintf(CPU_Usage1, sizeof CPU_Usage1, "%.0f%s", percent, "%");
        percent = ((double)systemtickfrequency - (double)idletick2) / (double)systemtickfrequency * 100;
        snprintf(CPU_Usage2, sizeof CPU_Usage2, "%.0f%s", percent, "%");
        percent = ((double)systemtickfrequency - (double)idletick3) / (double)systemtickfrequency * 100;
        snprintf(CPU_Usage3, sizeof CPU_Usage3, "%.0f%s", percent, "%");
        snprintf(CPU_compressed_c, sizeof CPU_compressed_c, "[%s,%s,%s,%s]@%.1f", CPU_Usage0, CPU_Usage1, CPU_Usage2, CPU_Usage3, (float)CPU_Hz / 1000000);

        ///GPU
        snprintf(GPU_Load_c, sizeof GPU_Load_c, "%.1f%s@%.1f", (float)GPU_Load_u / 10, "%", (float)GPU_Hz / 1000000);

        ///RAM
        float RAM_Total_application_f = (float)RAM_Total_application_u / 1024 / 1024;
        float RAM_Total_applet_f = (float)RAM_Total_applet_u / 1024 / 1024;
        float RAM_Total_system_f = (float)RAM_Total_system_u / 1024 / 1024;
        float RAM_Total_systemunsafe_f = (float)RAM_Total_systemunsafe_u / 1024 / 1024;
        float RAM_Total_all_f = RAM_Total_application_f + RAM_Total_applet_f + RAM_Total_system_f + RAM_Total_systemunsafe_f;
        float RAM_Used_application_f = (float)RAM_Used_application_u / 1024 / 1024;
        float RAM_Used_applet_f = (float)RAM_Used_applet_u / 1024 / 1024;
        float RAM_Used_system_f = (float)RAM_Used_system_u / 1024 / 1024;
        float RAM_Used_systemunsafe_f = (float)RAM_Used_systemunsafe_u / 1024 / 1024;
        float RAM_Used_all_f = RAM_Used_application_f + RAM_Used_applet_f + RAM_Used_system_f + RAM_Used_systemunsafe_f;
        snprintf(RAM_all_c, sizeof RAM_all_c, "%.0f/%.0fMB", RAM_Used_all_f, RAM_Total_all_f);
        snprintf(RAM_var_compressed_c, sizeof RAM_var_compressed_c, "%s@%.1f", RAM_all_c, (float)RAM_Hz / 1000000);

        ///Thermal
        snprintf(skin_temperature_c, sizeof skin_temperature_c, "%2.1f\u00B0C/%2.1f\u00B0C/%2.1f\u00B0C", (float)SoC_temperaturemiliC / 1000, (float)PCB_temperaturemiliC / 1000, (float)skin_temperaturemiliC / 1000);
        snprintf(Rotation_SpeedLevel_c, sizeof Rotation_SpeedLevel_c, "%2.2f%s", Rotation_SpeedLevel_f * 100, "%");

        ///FPS
        snprintf(FPS_c, sizeof FPS_c, "PFPS:");       //Pushed Frames Per Second
        snprintf(FPSavg_c, sizeof FPSavg_c, "FPS:");  //Frames Per Second calculated from averaged frametime
        snprintf(FPS_compressed_c, sizeof FPS_compressed_c, "%s\n%s", FPS_c, FPSavg_c);
        snprintf(FPS_var_compressed_c, sizeof FPS_compressed_c, "%u\n%2.2f", FPS, FPSavg);

        if (GameRunning == true)
            snprintf(Variables, sizeof Variables, "%s\n%s\n%s\n%s\n%s\n%s", CPU_compressed_c, GPU_Load_c, RAM_var_compressed_c, skin_temperature_c, Rotation_SpeedLevel_c, FPS_var_compressed_c);
        else
            snprintf(Variables, sizeof Variables, "%d\n%d\n%d\n%s\n%s", Bstate.A, Bstate.B, TeslaFPS, skin_temperature_c, Rotation_SpeedLevel_c);
    }
    virtual bool handleInput(u64 keysDown, u64 keysHeld, HidTouchState touchInput, JoystickPosition leftJoyStick, JoystickPosition rightJoyStick) override {
        if ((keysHeld & HidNpadButton_StickL) && (keysHeld & HidNpadButton_StickR)) {
            CloseThreads();
            tsl::goBack();
            return true;
        };
        if (keysDown & HidNpadButton_B && keysHeld & HidNpadButton_ZL) {
            CloseThreads();
            tsl::goBack();
            return true;
        }
        return false;
    }
};

// void get_outline() {  // WIP
//     char ss[200] = "";
//     for (auto entry : m_outline) {
//         snprintf(ss, sizeof ss, "[%s]\n", entry.label.c_str());
//         strcat(CheatsLabelsStr, ss);
//         snprintf(ss, sizeof ss, "%s\n", ((m_cheat_index == line) && (!m_show_only_enabled_cheats) && !m_cursor_on_bookmark) ? "\uE019" : "");
//         strcat(CheatsCursor, ss);
//         snprintf(ss, sizeof ss, "\n");
//         strcat(CheatsEnableStr, ss);
//     }
// };
void getcheats(){ // WIP
    m_displayed_cheat_lines = 0;
    char ss[200] = "";
    if (refresh_cheats) {
        if (m_cheatCnt != 0) delete m_cheats;
        dmntchtGetCheatCount(&m_cheatCnt);
        if (m_cheatCnt == 0) {
            loadcheatsfromfile();
            dmntchtGetCheatCount(&m_cheatCnt);
        }
        if (m_cheatCnt > 0) {
            m_cheats = new DmntCheatEntry[m_cheatCnt];
            dmntchtGetCheats(m_cheats, m_cheatCnt, 0, &m_cheatCnt);
        } else {
            CheatsLabelsStr[0] = 0;
            CheatsCursor[0] = 0;
            CheatsEnableStr[0] = 0;
            snprintf(CheatsEnableStr, sizeof CheatsEnableStr, "No Cheats available\n");
            cheat_outline_entry_t outline_entry = {};
            m_cheat_outline.push_back(outline_entry);
            return;
        }
        refresh_cheats = false;
        // make outline
        if (m_outline_refresh) {
        m_outline_refresh = false;    
        bool hide_entry = false;
        m_cheat_outline.clear();
        cheat_outline_entry_t cheat_outline_entry = {};
        for (u32 i = 0; i < m_cheatCnt; i++) {
            auto entry = m_cheats[i];
            if (entry.definition.opcodes[0] == 0x20000000 || entry.definition.opcodes[0] == 0x20000001) { // 0x20000001 is end outline code
                if (cheat_outline_entry.is_outline) {
                    cheat_outline_entry.size = i - cheat_outline_entry.index - 1;
                    m_cheat_outline.push_back(cheat_outline_entry);
                }
                cheat_outline_entry.index = i;
                cheat_outline_entry.is_outline = true;
                if (entry.definition.opcodes[0] == 0x20000000) {
                    cheat_outline_entry.always_expanded = false;
                    cheat_outline_entry.expanded = false;
                    hide_entry = true;
                } else {
                    cheat_outline_entry.expanded = true;
                    cheat_outline_entry.always_expanded = true;
                    if (show_outline_off) m_cheat_outline.push_back(cheat_outline_entry); // comment out this line to not show this type of outline
                    hide_entry = false;
                }
            } else if (!hide_entry) {
                cheat_outline_entry.index = i;
                cheat_outline_entry.is_outline = false;
                m_cheat_outline.push_back(cheat_outline_entry);
            }
        }
        if (cheat_outline_entry.is_outline) {
            cheat_outline_entry.size = m_cheatCnt - 1 - cheat_outline_entry.index;
            m_cheat_outline.push_back(cheat_outline_entry);
        };
        // if (m_cheat_outline.size() > 0)
        //     m_outline_mode = true;
        // else
        //     m_outline_mode = false;
        }
        //
    };
    if (m_outline_mode && !m_show_only_enabled_cheats) { // take over and ignore the rest
        CheatsLabelsStr[0] = 0;
        CheatsCursor[0] = 0;
        CheatsEnableStr[0] = 0;
        snprintf(CheatsLabelsStr, sizeof CheatsLabelsStr, "\n");
        snprintf(CheatsEnableStr, sizeof CheatsEnableStr, "\n");
        u32 Enable_count = 0;
        for (u8 i = 0; i < m_cheatCnt; i++)
            if (m_cheats[i].enabled) Enable_count ++;
        snprintf(CheatsCursor, sizeof CheatsCursor, "Cheat id = %d, Total enabled = %d/%ld\n", m_cheats[m_cheat_outline[m_cheat_index + m_cheatlist_offset].index].cheat_id, Enable_count, m_cheatCnt);
        for (u8 line = 0; line < NUM_cheats; line++) {
            if ((line + m_cheatlist_offset) >= m_cheat_outline.size())
                break;
            {
                DmntCheatEntry cheat_entry = m_cheats[m_cheat_outline[line + m_cheatlist_offset].index];
                char namestr[100] = "";
                char toggle_str[100] = "";
                if (!m_show_only_enabled_cheats) {
                    // sprintf(toggle_str, "size = %ld ", m_toggle_list.size());
                    for (size_t i = 0; i < m_toggle_list.size(); i++) {
                        if (m_toggle_list[i].cheat_id == m_cheats[line + m_cheatlist_offset].cheat_id) {
                            bool match = false;
                            for (u32 j = 0; j < buttonCodes.size(); j++) {
                                if ((m_toggle_list[i].keycode & buttonCodes[j]) == (buttonCodes[j] & 0x0FFFFFFF)) {
                                    strcat(toggle_str, buttonNames[j].c_str());
                                    match = true;
                                };
                            };
                            if (match) strcat(toggle_str, ", ");
                        }
                    }
                }
                int buttoncode = cheat_entry.definition.opcodes[0];
                if ((buttoncode & 0xF0000000) == 0x80000000)
                    for (u32 i = 0; i < buttonCodes.size(); i++) {
                        if ((buttoncode & buttonCodes[i]) == buttonCodes[i])
                            strcat(namestr, buttonNames[i].c_str());
                    }
                if ((m_cheat_index == line) && (m_editCheat) && !m_cursor_on_bookmark) {
                    snprintf(ss, sizeof ss, "Press key for combo count = %d\n", keycount);
                    strcat(CheatsLabelsStr, ss);
                    snprintf(ss, sizeof ss, "\n");
                    strcat(CheatsEnableStr, ss);
                } else {
                    if (cheat_entry.definition.opcodes[0] == 0x20000000) {
                        snprintf(ss, sizeof ss, "[%s%s %s] %d\n", namestr, cheat_entry.definition.readable_name, toggle_str, m_cheat_outline[line + m_cheatlist_offset].size);
                        strcat(CheatsLabelsStr, "\n");
                        strcat(CheatsEnableStr, ss);
                    } else if (cheat_entry.definition.opcodes[0] == 0x20000001) {
                        snprintf(ss, sizeof ss, "- outline off - [%s]\n", cheat_entry.definition.readable_name);
                        strcat(CheatsLabelsStr, "\n");
                        strcat(CheatsEnableStr, ss);
                    } else {
                        snprintf(ss, sizeof ss, "%s%s %s\n", namestr, cheat_entry.definition.readable_name, toggle_str);
                        strcat(CheatsLabelsStr, ss);
                        if (m_show_only_enabled_cheats)
                            snprintf(ss, sizeof ss, "\n");
                        else
                            snprintf(ss, sizeof ss, "%s\n", (cheat_entry.enabled) ? "\u25A0" : "\u25A1");
                        strcat(CheatsEnableStr, ss);
                    }
                }
                snprintf(ss, sizeof ss, "%s\n", ((m_cheat_index == line) && (!m_show_only_enabled_cheats) && !m_cursor_on_bookmark) ? "\uE019" : "");
                strcat(CheatsCursor, ss);
                m_displayed_cheat_lines++;
            }
        }
        return;
    }
    // End make outline
    if (m_show_only_enabled_cheats) {
        if (m_displayed_cheat_lines > 0) {
        snprintf(CheatsEnableStr, sizeof CheatsEnableStr, "\n");
        snprintf(CheatsCursor, sizeof CheatsCursor, "\n");
        snprintf(CheatsLabelsStr, sizeof CheatsLabelsStr, "Enabled Cheats %d\n",m_displayed_cheat_lines);
        m_displayed_cheat_lines = 0;
        } else {
            CheatsLabelsStr[0] = 0;
            CheatsCursor[0] = 0;
            CheatsEnableStr[0] = 0;
        }
    } else {
        total_opcode = 0;
        for (u8 i = 0; i < m_cheatCnt; i++)
            if (m_cheats[i].enabled) total_opcode += m_cheats[i].definition.num_opcodes;
        snprintf(CheatsCursor, sizeof CheatsCursor, "Cheats %d/%ld opcode = %d Total opcode = %d/%d\n", m_cheat_index + m_cheatlist_offset + 1, m_cheatCnt, m_cheats[m_cheat_index + m_cheatlist_offset].definition.num_opcodes, total_opcode, MaximumProgramOpcodeCount);
        snprintf(CheatsLabelsStr, sizeof CheatsLabelsStr, "\n");
        snprintf(CheatsEnableStr, sizeof CheatsEnableStr, "\n");
    };

//  print some extra outline label
    if (!ShowALlCheats->getState() && !m_show_only_enabled_cheats) {
        for (auto entry : m_outline) {
            if (entry.index + m_cheats[0].cheat_id < m_outline[m_outline_index].index) {
                snprintf(ss, sizeof ss, "[%s]\n", entry.label.c_str());
                strcat(CheatsLabelsStr, "\n");
                strcat(CheatsCursor, "\n");
                strcat(CheatsEnableStr, ss);
            } else
                break;
        }
    }
//
    for (u8 line = 0; line < NUM_cheats; line++) {
        while (m_show_only_enabled_cheats && !(m_cheats[line + m_cheatlist_offset].enabled)){
             m_cheatlist_offset ++;
             if ((line + m_cheatlist_offset) >= m_cheatCnt)
                 break;
        };
        if ((line + m_cheatlist_offset) >= m_cheatCnt)
            break;
        if (!ShowALlCheats->getState() && m_outline_index < (m_outline.size() - 1) && m_outline[m_outline_index + 1].index  < m_cheats[line + m_cheatlist_offset].cheat_id + 1 - m_cheats[0].cheat_id) break;
        {   
            char namestr[100] = "";
            char toggle_str[100] = "";
            if (!m_show_only_enabled_cheats) {
                // sprintf(toggle_str, "size = %ld ", m_toggle_list.size());
                for (size_t i = 0; i < m_toggle_list.size(); i++) {
                    if (m_toggle_list[i].cheat_id == m_cheats[line + m_cheatlist_offset].cheat_id) {
                        bool match = false;
                        for (u32 j = 0; j < buttonCodes.size(); j++) {
                            if ((m_toggle_list[i].keycode & buttonCodes[j]) == (buttonCodes[j] & 0x0FFFFFFF)) {
                                strcat(toggle_str, buttonNames[j].c_str());
                                match = true;
                            };
                        };
                        if (match) strcat(toggle_str, ", ");
                    }
                }
            }
            int buttoncode = m_cheats[line + m_cheatlist_offset].definition.opcodes[0];
            if ((buttoncode & 0xF0000000) == 0x80000000)
                for (u32 i = 0; i < buttonCodes.size(); i++) {
                    if ((buttoncode & buttonCodes[i]) == buttonCodes[i])
                        strcat(namestr, buttonNames[i].c_str());
                }
            if ((m_cheat_index == line) && (m_editCheat) && !m_cursor_on_bookmark) {
                snprintf(ss, sizeof ss, "Press key for combo count = %d\n", keycount);
                strcat(CheatsLabelsStr, ss);
                snprintf(ss, sizeof ss, "\n");
                strcat(CheatsEnableStr, ss);
            } else {
                // if (m_outline.size() > 1 && !m_show_only_enabled_cheats && ShowALlCheats->getState()) {
                //     for (auto entry : m_outline) {
                //         if (m_cheats[line + m_cheatlist_offset].cheat_id == entry.index + m_cheats[0].cheat_id) {
                //             snprintf(ss, sizeof ss, "[%s]\n", entry.label.c_str());
                //             strcat(CheatsLabelsStr, "\n");
                //             strcat(CheatsCursor, "\n");
                //             strcat(CheatsEnableStr, ss);
                //         }
                //         if (entry.index + m_cheats[0].cheat_id > m_cheats[line + m_cheatlist_offset].cheat_id) break;
                //     }
                // }
                if (m_cheats[line + m_cheatlist_offset].definition.opcodes[0] == 0x20000000) {
                    snprintf(ss, sizeof ss, "[%s%s %s]\n", namestr, m_cheats[line + m_cheatlist_offset].definition.readable_name, toggle_str);
                    strcat(CheatsLabelsStr, "\n");
                    strcat(CheatsEnableStr, ss);
                } else {
                    snprintf(ss, sizeof ss, "%s%s %s\n", namestr, m_cheats[line + m_cheatlist_offset].definition.readable_name, toggle_str);
                    strcat(CheatsLabelsStr, ss);
                    if (m_show_only_enabled_cheats)
                        snprintf(ss, sizeof ss, "\n");
                    else
                        snprintf(ss, sizeof ss, "%s\n", (m_cheats[line + m_cheatlist_offset].enabled) ? "\u25A0" : "\u25A1");
                    strcat(CheatsEnableStr, ss);
                }
            }
            snprintf(ss, sizeof ss, "%s\n", ((m_cheat_index == line) && (!m_show_only_enabled_cheats) && !m_cursor_on_bookmark) ? "\uE019" : "");
            strcat(CheatsCursor, ss);
            m_displayed_cheat_lines++;
        }
    };
    //  print some extra outline label
    if (!ShowALlCheats->getState() && !m_show_only_enabled_cheats) {
        for (auto entry : m_outline) {
            if (entry.index + m_cheats[0].cheat_id <= m_cheats[m_cheat_index + m_cheatlist_offset].cheat_id)
                continue;
            else {
                snprintf(ss, sizeof ss, "[%s]\n", entry.label.c_str());
                strcat(CheatsLabelsStr, "\n");
                strcat(CheatsCursor, "\n");
                strcat(CheatsEnableStr, ss);
            }
        }
    }
    //
};
class BookmarkOverlay : public tsl::Gui {
   public:
    BookmarkOverlay() {}

    virtual tsl::elm::Element *createUI() override {
        auto rootFrame = new tsl::elm::OverlayFrame("", "");

        auto Status = new tsl::elm::CustomDrawer([](tsl::gfx::Renderer *renderer, u16 x, u16 y, u16 w, u16 h) {
            // if (GameRunning == false)
            // fontsize = 20;
            std::pair<u32, u32> extent1;
            renderer->m_maxY = 0;
            // else
            extent1 = renderer->drawString(Title_str.c_str(), false, 5, fontsize +5 , fontsize, renderer->a(0xFFFF));
            //     renderer->drawRect(0, 0, tsl::cfg::FramebufferWidth - 150, 110, a(0x7111));
            renderer->m_maxX = 0;

            renderer->drawString(BookmarkLabels, false, 5, extent1.second + fontsize + 5, fontsize, renderer->a(0xFFFF));

            renderer->drawString(Variables, false, renderer->m_maxX + 5, extent1.second + fontsize + 5, fontsize, renderer->a(0xFFFF));

            renderer->drawString(MultiplierStr, false, renderer->m_maxX + 5, extent1.second + fontsize + 5, fontsize, renderer->a(0xFFFF));

        // strncat(BookmarkLabels,CheatsLabelsStr, sizeof BookmarkLabels-1);
            renderer->drawString(CheatsLabelsStr, false, 5, renderer->m_maxY, fontsize, renderer->a(0xFFFF));
            // renderer->drawRect(0, 0, std::max(5 + extent2.first, 190 + extent1.first) + 5, (fontsize + 3) * (2 + m_displayed_bookmark_lines + m_displayed_cheat_lines) + 5, a(0x7111));
            renderer->drawRect(0, 0, std::max(renderer->m_maxX, extent1.first + 5) + 5, renderer->m_maxY + 5 - fontsize, a(0x7111));
        });

        rootFrame->setContent(Status);

        return rootFrame;
    }

    virtual void update() override {
        if (TeslaFPS == 60) TeslaFPS = 1;
        if (m_on_show) {
            tsl::hlp::requestForeground(false);
            m_on_show = false;
        }
        // Check if game process has been terminated
        dmntchtHasCheatProcess(&(m_debugger->m_dmnt));
        if (!m_debugger->m_dmnt) {
            // cleanup_se_tools();
            tsl::goBack();
        };
        // snprintf(FPS_var_compressed_c, sizeof FPS_compressed_c, "%u\n%2.2f", FPS, FPSavg);
        // snprintf(BookmarkLabels,sizeof BookmarkLabels,"label\nlabe\nGame Runing = %d\n%s\nSaltySD = %d\ndmntchtCheck = 0x%08x\n",GameRunning,bookmarkfilename,SaltySD,dmntchtCheck);
        // snprintf(Variables,sizeof Variables, "100\n200\n\n\n\n\n");
        // strcat(BookmarkLabels,bookmarkfilename);
        BookmarkLabels[0] = 0;
        Variables[0] = 0;
        Cursor[0] = 0;
        MultiplierStr[0] = 0;
        // if (b_showkeyhint) {
        if (ShowKeyHints->getState()) {
            // snprintf(MultiplierStr, sizeof MultiplierStr, "\n");
            // snprintf(Variables, sizeof Variables, "\n");
            // snprintf(Cursor, sizeof Cursor, "\n");
            // snprintf(BookmarkLabels, sizeof BookmarkLabels, "\uE0A6+\uE0A4/\uE0A5 Font size  \uE0A6+\uE0A1 Exit\n");
            Title_str = "\uE0A6+\uE0A4/\uE0A5 Font size  \uE0A6+\uE0A1 Exit\n";
        } else {
            Title_str = "\uE0A6+\uE0A1 Exit\n";
        }
        m_displayed_bookmark_lines = 0;
        // BookmarkLabels[0]=0;
        // Variables[0]=0;
        // snprintf(Variables, sizeof Variables, "%d\n%d\n%d\n%s\n%s", Bstate.A, Bstate.B, TeslaFPS, skin_temperature_c, Rotation_SpeedLevel_c);
        for (u8 line = 0; line < NUM_bookmark; line++) {
            if ((line + m_addresslist_offset) >= (m_AttributeDumpBookmark->size() / sizeof(bookmark_t)))
                break;

            // std::stringstream ss;
            // ss.str("");
            char ss[200] = "";
            bookmark_t bookmark;
            // if (line < NUM_bookmark)  // && (m_memoryDump->size() / sizeof(u64)) != 8)
            {
                u64 address = 0;
                // m_memoryDump->getData((line + m_addresslist_offset) * sizeof(u64), &address, sizeof(u64));
                // return;
                m_AttributeDumpBookmark->getData((line + m_addresslist_offset) * sizeof(bookmark_t), &bookmark, sizeof(bookmark_t));
				if (bookmark.magic!=0x1289) bookmark.multiplier = 1;
                // if (false)
                if (bookmark.pointer.depth > 0)  // check if pointer chain point to valid address update address if necessary
                {
                    // bool updateaddress = true;
                    u64 nextaddress = metadata.main_nso_extents.base;  //m_mainBaseAddr;
                    for (int z = bookmark.pointer.depth; z >= 0; z--) {
                        nextaddress += bookmark.pointer.offset[z];
                        MemoryInfo meminfo = m_debugger->queryMemory(nextaddress);
                        if (meminfo.perm == Perm_Rw)
                            if (z == 0) {
                                if (address == nextaddress) {
                                    // updateaddress = false;
                                } else {
                                    address = nextaddress;
                                }
                            } else
                                m_debugger->readMemory(&nextaddress, ((m_32bitmode) ? sizeof(u32) : sizeof(u64)), nextaddress);
                        else {
                            // updateaddress = false;
                            break;
                        }
                    }
                    // if (updateaddress) {
                    // 	m_memoryDump->putData((line + m_addresslist_offset) * sizeof(u64), &address, sizeof(u64));
                    // 	m_memoryDump->flushBuffer();
                    // }
                } else {
                    address = ((bookmark.heap) ? ((m_debugger->queryMemory(metadata.heap_extents.base).type == 0) ? metadata.alias_extents.base
                                                                                                                  : metadata.heap_extents.base)
                                               : metadata.main_nso_extents.base) +
                              bookmark.offset;
                };
                // bookmark display
                // WIP
                searchValue_t value = {0};

                m_debugger->readMemory(&value, dataTypeSizes[bookmark.type], address);
                if ((m_oldvalue[line]._u64 == 0) || (m_oldvalue[line]._s64 > value._s64))
                    m_oldvalue[line]._s64 = value._s64;
                else if (bookmark.multiplier != 1) {
                    switch (bookmark.type) {
                        case SEARCH_TYPE_FLOAT_32BIT:
                            if (m_oldvalue[line]._f32 < value._f32) {
                                m_oldvalue[line]._f32 = (value._f32 - m_oldvalue[line]._f32) * bookmark.multiplier + m_oldvalue[line]._f32;
                            };
                            break;
                        case SEARCH_TYPE_FLOAT_64BIT:
                            if (m_oldvalue[line]._f64 < value._f64) {
                                m_oldvalue[line]._f64 = (value._f64 - m_oldvalue[line]._f64) * bookmark.multiplier + m_oldvalue[line]._f64;
                            };
                            break;
                        case SEARCH_TYPE_UNSIGNED_8BIT:
                        case SEARCH_TYPE_UNSIGNED_16BIT:
                        case SEARCH_TYPE_UNSIGNED_32BIT:
                        case SEARCH_TYPE_UNSIGNED_64BIT:
                            if (m_oldvalue[line]._u64 < value._u64) {
                                m_oldvalue[line]._u64 = (value._u64 - m_oldvalue[line]._u64) * bookmark.multiplier + m_oldvalue[line]._u64;
                            };
                            break;
                        default:
                            if (m_oldvalue[line]._s64 < value._s64) {
                                m_oldvalue[line]._s64 = (value._s64 - m_oldvalue[line]._s64) * bookmark.multiplier + m_oldvalue[line]._s64;
                            };
                            break;
                    };
                    m_debugger->writeMemory(&(m_oldvalue[line]), dataTypeSizes[bookmark.type], address);
                };
                snprintf(ss, sizeof ss, "%s\n", _getAddressDisplayString(address, m_debugger, (searchType_t)bookmark.type).c_str());
                strcat(Variables, ss);
                snprintf(ss, sizeof ss, "%s\n", bookmark.label);
                strcat(BookmarkLabels, ss);
                snprintf(ss, sizeof ss, "\n");
                strcat(Cursor, ss);
                snprintf(ss, sizeof ss, (bookmark.multiplier != 1) ? "X%02d\n" : "\n", bookmark.multiplier);
                strcat(MultiplierStr, ss);
                m_displayed_bookmark_lines++;
                // 	ss << "[0x" << std::uppercase << std::hex << std::setfill('0') << std::setw(10) << (address) << "]";  //<< std::left << std::setfill(' ') << std::setw(18) << bookmark.label <<

                // 	ss << "  ( " << _getAddressDisplayString(address, m_debugger, (searchType_t)bookmark.type) << " )";

                // 	if (m_frozenAddresses.find(address) != m_frozenAddresses.end())
                // 		ss << " \uE130";
                // 	if (bookmark.pointer.depth > 0)  // have pointer
                // 		ss << " *";
            }
            // else {
            // 	snprintf(ss, sizeof ss, "%s\n", bookmark.label);
            // 	strcat(BookmarkLabels,ss);
            // 	ss << "And " << std::dec << ((m_memoryDump->size() / sizeof(u64)) - 8) << " others...";
            // }
            // Gui::drawRectangle(Gui::g_framebuffer_width - 555, 300 + line * 40, 545, 40, (m_selectedEntry == line && m_menuLocation == CANDIDATES) ? currTheme.highlightColor : line % 2 == 0 ? currTheme.backgroundColor
            // 																																													: currTheme.separatorColor);
            // Gui::drawTextAligned(font14, Gui::g_framebuffer_width - 545, 305 + line * 40, (m_selectedEntry == line && m_menuLocation == CANDIDATES) ? COLOR_BLACK : currTheme.textColor, bookmark.deleted ? "To be deleted" : bookmark.label, ALIGNED_LEFT);
            // Gui::drawTextAligned(font14, Gui::g_framebuffer_width - 340, 305 + line * 40, (m_selectedEntry == line && m_menuLocation == CANDIDATES) ? COLOR_BLACK : currTheme.textColor, ss.str().c_str(), ALIGNED_LEFT);
        }
        m_show_only_enabled_cheats = true;
        m_cheatlist_offset = 0;
        getcheats();
        // strncat(BookmarkLabels,CheatsLabelsStr, sizeof BookmarkLabels-1);
        // strncat(MultiplierStr,CheatsEnableStr, sizeof MultiplierStr-1);
    };
    virtual bool handleInput(u64 keysDown, u64 keysHeld, HidTouchState touchInput, JoystickPosition leftJoyStick, JoystickPosition rightJoyStick) override {
        for (auto entry : m_toggle_list) {
            if (((keysHeld | keysDown) == entry.keycode) && (keysDown & entry.keycode)) {
                dmntchtToggleCheat(entry.cheat_id);
                refresh_cheats = true;
            }
        };
        if ((keysHeld & HidNpadButton_StickL) && (keysHeld & HidNpadButton_StickR)) {
            // CloseThreads();
            // cleanup_se_tools();
            m_cheatlist_offset = 0;
            TeslaFPS = 50;
            refreshrate = 1;
            alphabackground = 0x0;
            tsl::hlp::requestForeground(true);
            FullMode = false;
            tsl::goBack();
            return true;
        };
        if (keysDown & HidNpadButton_B && keysHeld & HidNpadButton_ZL) {
            // CloseThreads();
            // cleanup_se_tools();
            m_cheatlist_offset = m_cheatlist_offset_save;
            // if (m_outline.size() > 1)
            //     m_cheatlist_offset = m_outline[m_outline_index].index;
            // else
            //     m_cheatlist_offset = 0;
            TeslaFPS = 50;
            refreshrate = 1;
            alphabackground = 0x0;
            tsl::hlp::requestForeground(true);
            FullMode = false;
            dmntchtPauseCheatProcess();
            tsl::goBack();
            return true;
        }
        if (keysDown & HidNpadButton_R && keysHeld & HidNpadButton_ZL) {
            fontsize++;
            return true;
        }
        if (keysDown & HidNpadButton_L && keysHeld & HidNpadButton_ZL) {
            fontsize--;
            return true;
        }
        return false;
    }
};
//Mini mode
class MiniOverlay : public tsl::Gui {
   public:
    MiniOverlay() {}

    virtual tsl::elm::Element *createUI() override {
        auto rootFrame = new tsl::elm::OverlayFrame("", "");

        auto Status = new tsl::elm::CustomDrawer([](tsl::gfx::Renderer *renderer, u16 x, u16 y, u16 w, u16 h) {
            if (GameRunning == false)
                renderer->drawRect(0, 0, tsl::cfg::FramebufferWidth - 150, 80, a(0x7111));
            else
                renderer->drawRect(0, 0, tsl::cfg::FramebufferWidth - 150, 110, a(0x7111));

            //Print strings
            ///CPU
            if (GameRunning == true)
                renderer->drawString("CPU\nGPU\nRAM\nTEMP\nFAN\nPFPS\nFPS", false, 0, 15, 15, renderer->a(0xFFFF));
            else
                renderer->drawString("CPU\nGPU\nRAM\nTEMP\nFAN", false, 0, 15, 15, renderer->a(0xFFFF));

            ///GPU
            renderer->drawString(Variables, false, 60, 15, 15, renderer->a(0xFFFF));
        });

        rootFrame->setContent(Status);

        return rootFrame;
    }

    virtual void update() override {
        if (TeslaFPS == 60) TeslaFPS = 1;
        //In case of getting more than systemtickfrequency in idle, make it equal to systemtickfrequency to get 0% as output and nothing less
        //This is because making each loop also takes time, which is not considered because this will take also additional time
        if (idletick0 > systemtickfrequency) idletick0 = systemtickfrequency;
        if (idletick1 > systemtickfrequency) idletick1 = systemtickfrequency;
        if (idletick2 > systemtickfrequency) idletick2 = systemtickfrequency;
        if (idletick3 > systemtickfrequency) idletick3 = systemtickfrequency;

        //Make stuff ready to print
        ///CPU
        double percent = ((double)systemtickfrequency - (double)idletick0) / (double)systemtickfrequency * 100;
        snprintf(CPU_Usage0, sizeof CPU_Usage0, "%.0f%s", percent, "%");
        percent = ((double)systemtickfrequency - (double)idletick1) / (double)systemtickfrequency * 100;
        snprintf(CPU_Usage1, sizeof CPU_Usage1, "%.0f%s", percent, "%");
        percent = ((double)systemtickfrequency - (double)idletick2) / (double)systemtickfrequency * 100;
        snprintf(CPU_Usage2, sizeof CPU_Usage2, "%.0f%s", percent, "%");
        percent = ((double)systemtickfrequency - (double)idletick3) / (double)systemtickfrequency * 100;
        snprintf(CPU_Usage3, sizeof CPU_Usage3, "%.0f%s", percent, "%");
        snprintf(CPU_compressed_c, sizeof CPU_compressed_c, "[%s,%s,%s,%s]@%.1f", CPU_Usage0, CPU_Usage1, CPU_Usage2, CPU_Usage3, (float)CPU_Hz / 1000000);

        ///GPU
        snprintf(GPU_Load_c, sizeof GPU_Load_c, "%.1f%s@%.1f", (float)GPU_Load_u / 10, "%", (float)GPU_Hz / 1000000);

        ///RAM
        float RAM_Total_application_f = (float)RAM_Total_application_u / 1024 / 1024;
        float RAM_Total_applet_f = (float)RAM_Total_applet_u / 1024 / 1024;
        float RAM_Total_system_f = (float)RAM_Total_system_u / 1024 / 1024;
        float RAM_Total_systemunsafe_f = (float)RAM_Total_systemunsafe_u / 1024 / 1024;
        float RAM_Total_all_f = RAM_Total_application_f + RAM_Total_applet_f + RAM_Total_system_f + RAM_Total_systemunsafe_f;
        float RAM_Used_application_f = (float)RAM_Used_application_u / 1024 / 1024;
        float RAM_Used_applet_f = (float)RAM_Used_applet_u / 1024 / 1024;
        float RAM_Used_system_f = (float)RAM_Used_system_u / 1024 / 1024;
        float RAM_Used_systemunsafe_f = (float)RAM_Used_systemunsafe_u / 1024 / 1024;
        float RAM_Used_all_f = RAM_Used_application_f + RAM_Used_applet_f + RAM_Used_system_f + RAM_Used_systemunsafe_f;
        snprintf(RAM_all_c, sizeof RAM_all_c, "%.0f/%.0fMB", RAM_Used_all_f, RAM_Total_all_f);
        snprintf(RAM_var_compressed_c, sizeof RAM_var_compressed_c, "%s@%.1f", RAM_all_c, (float)RAM_Hz / 1000000);

        ///Thermal
        snprintf(skin_temperature_c, sizeof skin_temperature_c, "%2.1f\u00B0C/%2.1f\u00B0C/%2.1f\u00B0C", (float)SoC_temperaturemiliC / 1000, (float)PCB_temperaturemiliC / 1000, (float)skin_temperaturemiliC / 1000);
        snprintf(Rotation_SpeedLevel_c, sizeof Rotation_SpeedLevel_c, "%2.2f%s", Rotation_SpeedLevel_f * 100, "%");

        ///FPS
        snprintf(FPS_c, sizeof FPS_c, "PFPS:");       //Pushed Frames Per Second
        snprintf(FPSavg_c, sizeof FPSavg_c, "FPS:");  //Frames Per Second calculated from averaged frametime
        snprintf(FPS_compressed_c, sizeof FPS_compressed_c, "%s\n%s", FPS_c, FPSavg_c);
        snprintf(FPS_var_compressed_c, sizeof FPS_compressed_c, "%u\n%2.2f", FPS, FPSavg);

        if (GameRunning == true)
            snprintf(Variables, sizeof Variables, "%s\n%s\n%s\n%s\n%s\n%s", CPU_compressed_c, GPU_Load_c, RAM_var_compressed_c, skin_temperature_c, Rotation_SpeedLevel_c, FPS_var_compressed_c);
        else
            snprintf(Variables, sizeof Variables, "%s\n%s\n%s\n%s\n%s", CPU_compressed_c, GPU_Load_c, RAM_var_compressed_c, skin_temperature_c, Rotation_SpeedLevel_c);
    }
    virtual bool handleInput(u64 keysDown, u64 keysHeld, HidTouchState touchInput, JoystickPosition leftJoyStick, JoystickPosition rightJoyStick) override {
        if ((keysHeld & HidNpadButton_StickL) && (keysHeld & HidNpadButton_StickR)) {
            CloseThreads();
            tsl::goBack();
            return true;
        }
        return false;
    }
};

#ifdef CUSTOM
void BatteryChecker(void *) {
    if (R_SUCCEEDED(psmCheck)) {
        _batteryChargeInfoFields = new BatteryChargeInfoFields;
        while (!threadexit) {
            psmGetBatteryChargeInfoFields(psmService, _batteryChargeInfoFields);
            svcSleepThread(5'000'000'000);
        }
        delete _batteryChargeInfoFields;
    }
}

void StartBatteryThread() {
    threadCreate(&t7, BatteryChecker, NULL, NULL, 0x4000, 0x3F, 3);
    threadStart(&t7);
}

//CustomOverlay
class CustomOverlay : public tsl::Gui {
   public:
    CustomOverlay() {}

    virtual tsl::elm::Element *createUI() override {
        auto rootFrame = new tsl::elm::OverlayFrame("Custom Overlay", APP_VERSION);

        auto Status = new tsl::elm::CustomDrawer([](tsl::gfx::Renderer *renderer, u16 x, u16 y, u16 w, u16 h) {
            //Print strings
            ///CPU
            if (R_SUCCEEDED(clkrstCheck) || R_SUCCEEDED(pcvCheck)) {
                renderer->drawString("CPU Usage:", false, 20, 120, 20, renderer->a(0xFFFF));
                renderer->drawString(CPU_Hz_c, false, 20, 155, 15, renderer->a(0xFFFF));
                renderer->drawString(CPU_compressed_c, false, 20, 185, 15, renderer->a(0xFFFF));
            }

            ///GPU
            if (R_SUCCEEDED(clkrstCheck) || R_SUCCEEDED(pcvCheck) || R_SUCCEEDED(nvCheck)) {
                renderer->drawString("GPU Usage:", false, 20, 285, 20, renderer->a(0xFFFF));
                if (R_SUCCEEDED(clkrstCheck) || R_SUCCEEDED(pcvCheck)) renderer->drawString(GPU_Hz_c, false, 20, 320, 15, renderer->a(0xFFFF));
                if (R_SUCCEEDED(nvCheck)) renderer->drawString(GPU_Load_c, false, 20, 335, 15, renderer->a(0xFFFF));
            }

            ///RAM
            if (R_SUCCEEDED(psmCheck)) {
                renderer->drawString("Battery Stats:", false, 20, 375, 20, renderer->a(0xFFFF));
                renderer->drawString(Battery_c, false, 20, 410, 15, renderer->a(0xFFFF));
            }

            ///Thermal
            if (R_SUCCEEDED(tsCheck) || R_SUCCEEDED(tcCheck) || R_SUCCEEDED(fanCheck)) {
                renderer->drawString("Thermal:", false, 20, 540, 20, renderer->a(0xFFFF));
                if (R_SUCCEEDED(tsCheck)) renderer->drawString(SoCPCB_temperature_c, false, 20, 575, 15, renderer->a(0xFFFF));
                if (R_SUCCEEDED(tcCheck)) renderer->drawString(skin_temperature_c, false, 20, 605, 15, renderer->a(0xFFFF));
                if (R_SUCCEEDED(fanCheck)) renderer->drawString(Rotation_SpeedLevel_c, false, 20, 620, 15, renderer->a(0xFFFF));
            }

            if (refreshrate == 5)
                renderer->drawString("Hold Left Stick & Right Stick to Exit\nHold ZR + R + D-Pad Down to slow down refresh", false, 20, 675, 15, renderer->a(0xFFFF));
            else if (refreshrate == 1)
                renderer->drawString("Hold Left Stick & Right Stick to Exit\nHold ZR + R + D-Pad Up to speed up refresh", false, 20, 675, 15, renderer->a(0xFFFF));
        });

        rootFrame->setContent(Status);

        return rootFrame;
    }

    virtual void update() override {
        if (TeslaFPS == 60) TeslaFPS = 1;
        //In case of getting more than systemtickfrequency in idle, make it equal to systemtickfrequency to get 0% as output and nothing less
        //This is because making each loop also takes time, which is not considered because this will take also additional time
        if (idletick0 > systemtickfrequency) idletick0 = systemtickfrequency;
        if (idletick1 > systemtickfrequency) idletick1 = systemtickfrequency;
        if (idletick2 > systemtickfrequency) idletick2 = systemtickfrequency;
        if (idletick3 > systemtickfrequency) idletick3 = systemtickfrequency;

        //Make stuff ready to print
        ///CPU
        snprintf(CPU_Hz_c, sizeof CPU_Hz_c, "Frequency: %.1f MHz", (float)CPU_Hz / 1000000);
        snprintf(CPU_Usage0, sizeof CPU_Usage0, "Core #0: %.2f%s", ((double)systemtickfrequency - (double)idletick0) / (double)systemtickfrequency * 100, "%");
        snprintf(CPU_Usage1, sizeof CPU_Usage1, "Core #1: %.2f%s", ((double)systemtickfrequency - (double)idletick1) / (double)systemtickfrequency * 100, "%");
        snprintf(CPU_Usage2, sizeof CPU_Usage2, "Core #2: %.2f%s", ((double)systemtickfrequency - (double)idletick2) / (double)systemtickfrequency * 100, "%");
        snprintf(CPU_Usage3, sizeof CPU_Usage3, "Core #3: %.2f%s", ((double)systemtickfrequency - (double)idletick3) / (double)systemtickfrequency * 100, "%");
        snprintf(CPU_compressed_c, sizeof CPU_compressed_c, "%s\n%s\n%s\n%s", CPU_Usage0, CPU_Usage1, CPU_Usage2, CPU_Usage3);

        ///GPU
        snprintf(GPU_Hz_c, sizeof GPU_Hz_c, "Frequency: %.1f MHz", (float)GPU_Hz / 1000000);
        snprintf(GPU_Load_c, sizeof GPU_Load_c, "Load: %.1f%s", (float)GPU_Load_u / 10, "%");

        ///Battery
        snprintf(Battery_c, sizeof Battery_c,
                 "Battery Temperature: %.1f\u00B0C\n"
                 "Raw Battery Charge: %.1f%s\n"
                 "Voltage Avg: %u mV\n"
                 "Charger Type: %u\n",
                 (float)_batteryChargeInfoFields->BatteryTemperature / 1000,
                 (float)_batteryChargeInfoFields->RawBatteryCharge / 1000, "%",
                 _batteryChargeInfoFields->VoltageAvg,
                 _batteryChargeInfoFields->ChargerType);

        ///Thermal
        snprintf(SoCPCB_temperature_c, sizeof SoCPCB_temperature_c, "SoC: %2.2f \u00B0C\nPCB: %2.2f \u00B0C", (float)SoC_temperaturemiliC / 1000, (float)PCB_temperaturemiliC / 1000);
        snprintf(skin_temperature_c, sizeof skin_temperature_c, "Skin: %2.2f \u00B0C", (float)skin_temperaturemiliC / 1000);
        snprintf(Rotation_SpeedLevel_c, sizeof Rotation_SpeedLevel_c, "Fan: %2.2f%s", Rotation_SpeedLevel_f * 100, "%");
    }
    virtual bool handleInput(uint64_t keysDown, uint64_t keysHeld, HidTouchState touchInput, JoystickPosition leftJoyStick, JoystickPosition rightJoyStick) override {
        if ((keysHeld & HidNpadButton_StickL) && (keysHeld & HidNpadButton_RSTICK)) {
            CloseThreads();
            tsl::goBack();
            return true;
        }
        return false;
    }
};
#endif
// u32 temp_maxY;
class SetMultiplierOverlay : public tsl::Gui {
   public:
    SetMultiplierOverlay() {
        m_cheatlist_offset = m_cheatlist_offset_save;
        if (m_outline.size() <= 1) ShowALlCheats->setState(true);
    }
    virtual tsl::elm::Element *createUI() override {
        auto rootFrame = new tsl::elm::OverlayFrame("", "");

        auto Status = new tsl::elm::CustomDrawer([](tsl::gfx::Renderer *renderer, u16 x, u16 y, u16 w, u16 h) {
            // if (GameRunning == false)
            // fontsize = 15;
            renderer->drawRect(0, 0, tsl::cfg::FramebufferWidth , tsl::cfg::FramebufferHeight, a(0x7111));
            // else
            //     renderer->drawRect(0, 0, tsl::cfg::FramebufferWidth - 150, 110, a(0x7111));
            renderer->m_maxY = 0;
            renderer->drawString(BookmarkLabels, false, 65, fontsize, fontsize, renderer->a(0xFFFF));

            renderer->drawString(Variables, false, 210, fontsize, fontsize, renderer->a(0xFFFF));

            renderer->drawString(Cursor, false, 5, fontsize, fontsize, renderer->a(0xFFFF));

            renderer->drawString(MultiplierStr, false, 25, fontsize, fontsize, renderer->a(0xFFFF));
            // temp_maxY = renderer->m_maxY;
            // if (m_displayed_cheat_lines == m_NUM_cheats)
            // if (m_NUM_cheats < MAX_NUM_cheats)
            m_NUM_cheats = std::min(m_displayed_cheat_lines + (s32)(tsl::cfg::FramebufferHeight - renderer->m_maxY) / (fontsize + 3), (u32)MAX_NUM_cheats);

            // if (m_NUM_cheats > MAX_NUM_cheats) m_NUM_cheats = MAX_NUM_cheats;
        });

        rootFrame->setContent(Status);

        return rootFrame;
    }

    virtual void update() override {
        // if (TeslaFPS == 60) TeslaFPS = 1;

        // snprintf(FPS_var_compressed_c, sizeof FPS_compressed_c, "%u\n%2.2f", FPS, FPSavg);
        // snprintf(BookmarkLabels,sizeof BookmarkLabels,"label\nlabe\nGame Runing = %d\n%s\nSaltySD = %d\ndmntchtCheck = 0x%08x\n",GameRunning,bookmarkfilename,SaltySD,dmntchtCheck);
        // snprintf(Variables,sizeof Variables, "100\n200\n\n\n\n\n");
        // strcat(BookmarkLabels,bookmarkfilename);
        if (ShowKeyHints->getState()) {
            if (m_cursor_on_bookmark){
                snprintf(Cursor, sizeof Cursor, "%s %s, PID %03ld, \uE0A6+\uE0A1 Goto Monitor\nTID %016lX, BID %02X%02X%02X%02X%02X%02X%02X%02X\n\uE092\uE093\uE091\uE090 nav, \uE0A4 \uE0A5 change, \uE0A0 edit, \uE0A1 exit, \uE0A6+\uE0A4/\uE0A5 Font size\n\uE0B4 delete, \uE0A6\uE0B3 remove Key hint\n\uE0A6+\uE04E toggle show all cheats, \uE0A3 find next enabled cheat\n",
                         m_titleName.c_str(), m_versionString.c_str(), metadata.process_id, metadata.title_id, build_id[0], build_id[1], build_id[2], build_id[3], build_id[4], build_id[5], build_id[6], build_id[7]);
                snprintf(BookmarkLabels, sizeof BookmarkLabels, "\n\n\n\n\n");
                snprintf(Variables, sizeof Variables, "\n\n\n\n\n");
                snprintf(MultiplierStr, sizeof MultiplierStr, "\n\n\n\n\n");
            } else {
                snprintf(Cursor, sizeof Cursor, "%s %s, PID %03ld, \uE0A6+\uE0A1 Goto Monitor\nTID %016lX, BID %02X%02X%02X%02X%02X%02X%02X%02X\n\uE092\uE093\uE091\uE090\uE0A4\uE0A5 nav, \uE0A0 toggle, \uE0B3 add bookmark, \uE0B4 delete, \uE0A1 exit\n\uE0C5 Change/Add combo key, \uE0A6+\uE0C5 Remove combo key\n\uE0C4 Change/Add toggle key, \uE0A6+\uE0C4 Remove toggle key\n",
                         m_titleName.c_str(), m_versionString.c_str(), metadata.process_id, metadata.title_id, build_id[0], build_id[1], build_id[2], build_id[3], build_id[4], build_id[5], build_id[6], build_id[7]);
                snprintf(BookmarkLabels, sizeof BookmarkLabels, "\n\n\n\n\n");
                snprintf(Variables, sizeof Variables, "\n\n\n\n\n");
                snprintf(MultiplierStr, sizeof MultiplierStr, "\n\n\n\n\n");
            }
        } else {
            // BookmarkLabels[0] = 0;
            // Variables[0] = 0;
            // Cursor[0] = 0;
            // MultiplierStr[0] = 0;
            snprintf(BookmarkLabels, sizeof BookmarkLabels, "\n\n");
            snprintf(Variables, sizeof Variables, "\n\n");
// snprintf(Cursor, sizeof Cursor, "FramebufferHeight = %d, m_maxY = %d\n m=%d = %d + %d \n",tsl::cfg::FramebufferHeight, temp_maxY,m_NUM_cheats,m_displayed_cheat_lines, (s32)(tsl::cfg::FramebufferHeight - temp_maxY) / (fontsize + 3));
            snprintf(Cursor, sizeof Cursor, "%s %s, PID %03ld, \uE0A6\uE0B3 Key hint\nTID %016lX, BID %02X%02X%02X%02X%02X%02X%02X%02X\n", m_titleName.c_str(), m_versionString.c_str(), metadata.process_id, metadata.title_id, build_id[0], build_id[1], build_id[2], build_id[3], build_id[4], build_id[5], build_id[6], build_id[7]);
            snprintf(MultiplierStr, sizeof MultiplierStr, "\n\n");
        }
        // BookmarkLabels[0]=0;
        // Variables[0]=0;
        // snprintf(Variables, sizeof Variables, "%d\n%d\n%d\n%s\n%s", Bstate.A, Bstate.B, TeslaFPS, skin_temperature_c, Rotation_SpeedLevel_c);
        for (u8 line = 0; line < NUM_bookmark; line++) {
            if ((line + m_addresslist_offset) >= (m_AttributeDumpBookmark->size() / sizeof(bookmark_t)))
                break;

            // std::stringstream ss;
            // ss.str("");
            char ss[200] = "";
            bookmark_t bookmark;

            // if (line < NUM_bookmark)  // && (m_memoryDump->size() / sizeof(u64)) != 8)
            {
                u64 address = 0;
                // m_memoryDump->getData((line + m_addresslist_offset) * sizeof(u64), &address, sizeof(u64));
                // return;
                m_AttributeDumpBookmark->getData((line + m_addresslist_offset) * sizeof(bookmark_t), &bookmark, sizeof(bookmark_t));
				if (bookmark.magic!=0x1289) bookmark.multiplier = 1;
                // if (false)
                if (bookmark.pointer.depth > 0)  // check if pointer chain point to valid address update address if necessary
                {
                    // bool updateaddress = true;
                    u64 nextaddress = metadata.main_nso_extents.base;  //m_mainBaseAddr;
                    for (int z = bookmark.pointer.depth; z >= 0; z--) {
                        nextaddress += bookmark.pointer.offset[z];
                        MemoryInfo meminfo = m_debugger->queryMemory(nextaddress);
                        if (meminfo.perm == Perm_Rw)
                            if (z == 0) {
                                if (address == nextaddress) {
                                    // updateaddress = false;
                                } else {
                                    address = nextaddress;
                                }
                            } else
                                m_debugger->readMemory(&nextaddress, ((m_32bitmode) ? sizeof(u32) : sizeof(u64)), nextaddress);
                        else {
                            // updateaddress = false;
                            break;
                        }
                    }
                    // if (updateaddress) {
                    //     m_memoryDump->putData((line + m_addresslist_offset) * sizeof(u64), &address, sizeof(u64));
                    //     m_memoryDump->flushBuffer();
                    // }
                } else {
                    address = ((bookmark.heap) ? ((m_debugger->queryMemory(metadata.heap_extents.base).type == 0) ? metadata.alias_extents.base
                                                                                                                  : metadata.heap_extents.base)
                                               : metadata.main_nso_extents.base) +
                              bookmark.offset;
                };
                // bookmark display
                snprintf(ss, sizeof ss, "%s\n", ((m_index == line) && m_edit_value) ? valueStr_edit_display(Display).c_str() : _getAddressDisplayString(address, m_debugger, (searchType_t)bookmark.type).c_str());
                if (m_index == line) {
                    m_selected_address = address;
                    m_selected_type = bookmark.type;
                };
                if ((m_index == line) && (m_editCheat) && m_cursor_on_bookmark) {
                    strcat(Variables, "\n");
                    snprintf(ss, sizeof ss, "Press key for combo count = %d\n", keycount);
                } else {
                    char toggle_str[100] = "";
                    for (auto entry : m_breeze_action_list) {
                        if (entry.index == line) {
                            for (u32 j = 0; j < buttonCodes.size(); j++) {
                                if ((entry.keycode & buttonCodes[j]) == (buttonCodes[j] & 0x0FFFFFFF)) {
                                    strcat(toggle_str, buttonNames[j].c_str());
                                };
                            };
                            strcat(toggle_str, ", ");
                        }
                    }
                    if (strlen(toggle_str) != 0) {
                        strcat(Variables, "\n");
                        snprintf(ss, sizeof ss, "%s %s\n", bookmark.label, toggle_str);
                    } else {
                        strcat(Variables, ss);
                        snprintf(ss, sizeof ss, "%s\n", bookmark.label);
                    }
                }
                strcat(BookmarkLabels, ss);
                snprintf(ss, sizeof ss, "%s\n", ((m_index == line) && m_cursor_on_bookmark) ? "\uE019" : "");
                strcat(Cursor, ss);
                snprintf(ss, sizeof ss, "X%02d\n", bookmark.multiplier);
                strcat(MultiplierStr, ss);
                // 	ss << "[0x" << std::uppercase << std::hex << std::setfill('0') << std::setw(10) << (address) << "]";  //<< std::left << std::setfill(' ') << std::setw(18) << bookmark.label <<

                // 	ss << "  ( " << _getAddressDisplayString(address, m_debugger, (searchType_t)bookmark.type) << " )";

                // 	if (m_frozenAddresses.find(address) != m_frozenAddresses.end())
                // 		ss << " \uE130";
                // 	if (bookmark.pointer.depth > 0)  // have pointer
                // 		ss << " *";
            }
            // else {
            // 	snprintf(ss, sizeof ss, "%s\n", bookmark.label);
            // 	strcat(BookmarkLabels,ss);
            // 	ss << "And " << std::dec << ((m_memoryDump->size() / sizeof(u64)) - 8) << " others...";
            // }
            // Gui::drawRectangle(Gui::g_framebuffer_width - 555, 300 + line * 40, 545, 40, (m_selectedEntry == line && m_menuLocation == CANDIDATES) ? currTheme.highlightColor : line % 2 == 0 ? currTheme.backgroundColor
            // 																																													: currTheme.separatorColor);
            // Gui::drawTextAligned(font14, Gui::g_framebuffer_width - 545, 305 + line * 40, (m_selectedEntry == line && m_menuLocation == CANDIDATES) ? COLOR_BLACK : currTheme.textColor, bookmark.deleted ? "To be deleted" : bookmark.label, ALIGNED_LEFT);
            // Gui::drawTextAligned(font14, Gui::g_framebuffer_width - 340, 305 + line * 40, (m_selectedEntry == line && m_menuLocation == CANDIDATES) ? COLOR_BLACK : currTheme.textColor, ss.str().c_str(), ALIGNED_LEFT);
        }
        if (m_edit_value) {
            strncat(Cursor, "\uE092\uE093\uE0A4\uE0A5 \uE0A0 Select, \uE0A1 Backspace, \uE045 Enter, \uE0A2 Cancel\n", sizeof Cursor - 1);
            strncat(MultiplierStr, "\n", sizeof MultiplierStr - 1);
            for (u8 i = 0; i < keyNames.size(); i++) {
                strncat(Cursor, (i == m_value_edit_index) ? "->\n" : "\n", sizeof Cursor - 1);
                strncat(MultiplierStr, (keyNames[i]+"\n").c_str(), sizeof MultiplierStr - 1);
            }
            return;
        }
        if (m_show_outline) {
            strncat(Cursor, "Cheats outline \uE092\uE093 \uE0A0 Select\n", sizeof Cursor - 1); //, \uE0A6+\uE0A7 Toggle outline mode
            strncat(BookmarkLabels, "\n", sizeof BookmarkLabels - 1);
            strncat(MultiplierStr, "\n", sizeof MultiplierStr - 1);
            for (u8 i = 0; i < m_outline.size(); i++) {
                strncat(Cursor, (i == m_outline_index && !m_cursor_on_bookmark) ? "\uE019\n" : "\n", sizeof Cursor - 1);
                strncat(MultiplierStr, ("[" + m_outline[i].label + "]\n").c_str(), sizeof MultiplierStr - 1);
            }
            return;
        }
        // print cheats
        if (m_AttributeDumpBookmark->size() == 0) m_cursor_on_bookmark = false;
        m_show_only_enabled_cheats = false;
        // m_cheatlist_offset = 0;
        getcheats();
        strncat(BookmarkLabels,CheatsLabelsStr, sizeof BookmarkLabels-1);
        strncat(Cursor, CheatsCursor, sizeof Cursor - 1);
        strncat(MultiplierStr,CheatsEnableStr, sizeof MultiplierStr-1);
    };
    // WIP Setting
    virtual bool handleInput(u64 keysDown, u64 keysHeld, HidTouchState touchInput, JoystickPosition leftJoyStick, JoystickPosition rightJoyStick) override {
        static u32 keycode;
        u32 redirect_index = m_cheat_index + m_cheatlist_offset;
        if (m_outline_mode) redirect_index = m_cheat_outline[redirect_index].index;
        for (auto entry : m_toggle_list) {
            if (((keysHeld | keysDown) == entry.keycode) && (keysDown & entry.keycode)) {
                dmntchtToggleCheat(entry.cheat_id);
                refresh_cheats = true;
            }
        };
        if (m_editCheat) {
            if (keysDown == 0) return false;
            keycode = keycode | keysDown;  // Waitforkey_menu will send keycode in index to this buttonid;
            keycount--;
            if (keycount > 0) return true;
            m_editCheat = false;
            if (m_get_toggle_keycode) {
                toggle_list_t entry;
                entry.cheat_id = m_cheats[redirect_index].cheat_id;
                entry.keycode = keycode;
                m_toggle_list.push_back(entry);
                m_get_toggle_keycode = false;
            } else if (m_get_action_keycode) {
                // setup to get the rest of the required data

                breeze_action_list_t entry;
                entry.index = m_index;
                entry.keycode = keycode;
                entry.breeze_action = Set;
                entry.freeze_value._u16 = 10;
                m_breeze_action_list.push_back(entry); // temp test code

                m_get_action_keycode = false;
            } else {
                // edit cheat
                if ((m_cheats[redirect_index].definition.opcodes[0] & 0xF0000000) == 0x80000000) {
                    m_cheats[redirect_index].definition.opcodes[0] = keycode;
                } else {
                    if (m_cheats[redirect_index].definition.num_opcodes < 0x100 + 2) {
                        m_cheats[redirect_index].definition.opcodes[m_cheats[redirect_index].definition.num_opcodes + 1] = 0x20000000;

                        for (u32 i = m_cheats[redirect_index].definition.num_opcodes; i > 0; i--) {
                            m_cheats[redirect_index].definition.opcodes[i] = m_cheats[redirect_index].definition.opcodes[i - 1];
                        }
                        m_cheats[redirect_index].definition.num_opcodes += 2;
                        m_cheats[redirect_index].definition.opcodes[0] = keycode;
                    }
                }
                // modify cheat
                dmntchtRemoveCheat(m_cheats[redirect_index].cheat_id);
                u32 outid = 0;
                dmntchtAddCheat(&(m_cheats[redirect_index].definition), m_cheats[redirect_index].enabled, &outid);
                refresh_cheats = true;
            };
            return true;
        };
        if (m_edit_value) {
            if (keysDown & HidNpadButton_L) {
                if (value_pos > 0) value_pos--;
                return true;
            }
            if (keysDown & HidNpadButton_R) {
                if (value_pos < valueStr.length()) value_pos++;
                return true;
            }
            if (keysDown & HidNpadButton_A) {
                valueStr_edit_display(Insert);
                value_pos++;
                return true;
            }
            if (keysDown & HidNpadButton_Plus) {
                searchValue_t searchValue;
                if (m_hex_mode == false)
                    switch (m_selected_type) {
                        case SEARCH_TYPE_FLOAT_32BIT:
                            searchValue._f32 = static_cast<float>(std::atof(valueStr.c_str()));
                            break;
                        case SEARCH_TYPE_FLOAT_64BIT:
                            searchValue._f64 = std::atof(valueStr.c_str());
                            break;
                        case SEARCH_TYPE_UNSIGNED_8BIT:
                            searchValue._u8 = std::atol(valueStr.c_str());
                            break;
                        case SEARCH_TYPE_SIGNED_8BIT:
                            searchValue._s8 = std::atol(valueStr.c_str());
                            break;
                        case SEARCH_TYPE_UNSIGNED_16BIT:
                            searchValue._u16 = std::atol(valueStr.c_str());
                            break;
                        case SEARCH_TYPE_SIGNED_16BIT:
                            searchValue._s16 = std::atol(valueStr.c_str());
                            break;
                        case SEARCH_TYPE_UNSIGNED_32BIT:
                            searchValue._u32 = std::atol(valueStr.c_str());
                            break;
                        case SEARCH_TYPE_SIGNED_32BIT:
                            searchValue._s32 = std::atol(valueStr.c_str());
                            break;
                        case SEARCH_TYPE_UNSIGNED_64BIT:
                            searchValue._u64 = std::atol(valueStr.c_str());
                            break;
                        case SEARCH_TYPE_SIGNED_64BIT:
                            searchValue._s64 = std::atol(valueStr.c_str());
                            break;
                        default:
                            searchValue._u64 = std::atol(valueStr.c_str());
                            break;
                    }
                else
                    switch (m_selected_type) {
                        case SEARCH_TYPE_FLOAT_32BIT:
                            searchValue._f32 = static_cast<float>(std::atof(valueStr.c_str()));
                            break;
                        case SEARCH_TYPE_FLOAT_64BIT:
                            searchValue._f64 = std::atof(valueStr.c_str());
                            break;
                        case SEARCH_TYPE_UNSIGNED_8BIT:
                            searchValue._u8 = std::strtoul(valueStr.c_str(), NULL, 16);
                            break;
                        case SEARCH_TYPE_SIGNED_8BIT:
                            searchValue._s8 = std::strtoul(valueStr.c_str(), NULL, 16);
                            break;
                        case SEARCH_TYPE_UNSIGNED_16BIT:
                            searchValue._u16 = std::strtoul(valueStr.c_str(), NULL, 16);
                            break;
                        case SEARCH_TYPE_SIGNED_16BIT:
                            searchValue._s16 = std::strtoul(valueStr.c_str(), NULL, 16);
                            break;
                        case SEARCH_TYPE_UNSIGNED_32BIT:
                            searchValue._u32 = std::strtoul(valueStr.c_str(), NULL, 16);
                            break;
                        case SEARCH_TYPE_SIGNED_32BIT:
                            searchValue._s32 = std::strtoul(valueStr.c_str(), NULL, 16);
                            break;
                        case SEARCH_TYPE_UNSIGNED_64BIT:
                            searchValue._u64 = std::strtoul(valueStr.c_str(), NULL, 16);
                            break;
                        case SEARCH_TYPE_SIGNED_64BIT:
                            searchValue._s64 = std::strtoul(valueStr.c_str(), NULL, 16);
                            break;
                        default:
                            searchValue._u64 = std::strtoul(valueStr.c_str(), NULL, 16);
                            break;
                    }
                m_debugger->writeMemory(&searchValue, dataTypeSizes[m_selected_type], m_selected_address);
                m_edit_value = false;
                return true;
            }
            if (keysDown & HidNpadButton_B) {
                if (value_pos > 0) {
                    valueStr_edit_display(Delete);
                    value_pos--;
                }
                return true;
            }
            if ((keysDown & HidNpadButton_AnyUp) || (keysHeld & HidNpadButton_StickRUp)) {
                if (m_value_edit_index > 0) m_value_edit_index--;
                return true;
            };
            if ((keysDown & HidNpadButton_AnyDown) || (keysHeld & HidNpadButton_StickRDown)) {
                if (m_value_edit_index < keyNames.size() - 1) m_value_edit_index++;
                return true;
            };
            if (keysDown & HidNpadButton_X) {
                m_edit_value = false;
                return true;
            };
            return true;
        }
        if (keysDown & HidNpadButton_Plus && keysHeld & HidNpadButton_ZL) {
            ShowKeyHints->setState(!ShowKeyHints->getState());
        };
        if (keysDown & HidNpadButton_B && keysHeld & HidNpadButton_ZL) {
            TeslaFPS = 20;
            refreshrate = 1;
            alphabackground = 0x0;
            tsl::hlp::requestForeground(false);
            FullMode = false;
            tsl::changeTo<BookmarkOverlay>();
            dmntchtResumeCheatProcess();
            if (save_code_to_file) dumpcodetofile();
            save_code_to_file = false;
            if (save_breeze_toggle_to_file) save_breeze_toggle();
            save_breeze_toggle_to_file = false;
            if (save_breeze_action_to_file) save_breeze_action();
            save_breeze_action_to_file = false;
            m_cheatlist_offset_save = m_cheatlist_offset;
            return true;
        }
        // if ((keysHeld & HidNpadButton_ZL) && (keysDown & HidNpadButton_ZR)) {  // enter outline mode
        //     m_show_outline = !m_show_outline;
        //     if (m_show_outline) m_cursor_on_bookmark = false;
        //     return true;
        // }
        if (keysDown & HidNpadButton_R && keysHeld & HidNpadButton_ZL) {
            fontsize++;
            return true;
        }
        if (keysDown & HidNpadButton_L && keysHeld & HidNpadButton_ZL) {
            fontsize--;
            return true;
        }
        if (keysDown & HidNpadButton_AnyLeft) {
            if (m_cursor_on_bookmark && m_cheatCnt > 0) {
                m_show_outline = false;
                m_cursor_on_bookmark = false;
            } else if (m_show_outline) {
                if (m_AttributeDumpBookmark->size() > 0) {
                    m_cursor_on_bookmark = true;
                }
            } else if (m_outline.size() > 1 && !m_outline_mode) 
                m_show_outline = true;
            else if (m_AttributeDumpBookmark->size() > 0) {
                m_cursor_on_bookmark = true;
            }
            return true;
        }
        if (keysDown & HidNpadButton_AnyRight) {
            if (m_cursor_on_bookmark && m_cheatCnt > 0) {
                if (m_outline.size() > 1 && !m_outline_mode) m_show_outline = true;
                m_cursor_on_bookmark = false;
            } else if (m_show_outline) {
                m_cheatlist_offset = m_outline[m_outline_index].index;
                m_cheat_index = 0;
                m_show_outline = false;
            } else if (m_AttributeDumpBookmark->size() > 0) {
                m_cursor_on_bookmark = true;
            }
            return true;
        }
        if (keysDown & HidNpadButton_X && !(keysHeld & HidNpadButton_ZL)) {
            if (m_outline_mode) return true;
            if (m_outline.size() > 1) {
                ShowALlCheats->setState(!ShowALlCheats->getState());
                m_cheatlist_offset = m_outline[m_outline_index].index;
                m_cheat_index = 0;
            }
            return true;
        }
        if (keysDown & HidNpadButton_X && (keysHeld & HidNpadButton_ZL)) {
            m_outline_mode = !m_outline_mode;
            if (m_outline_mode) {
                ShowALlCheats->setState(true);
                // m_cheat_index = 0;
                // m_cheatlist_offset = 0;
                m_cheat_index = m_cheat_index_save;
                m_cheatlist_offset = m_cheatlist_offset_save;
            } else {
                m_cheat_index_save = m_cheat_index;
                m_cheatlist_offset_save = m_cheatlist_offset;
                m_cheat_index = 0;
                m_cheatlist_offset = redirect_index;
            }
            return true;
        }
        if (keysDown & HidNpadButton_Y) {  //find next enabled cheat
            m_show_outline = false;
            m_outline_mode = false;
            m_cursor_on_bookmark = false;
            ShowALlCheats->setState(true);
            // getcheats();
            // if (total_opcode == 0) return true;
            if ((m_cheat_index + m_cheatlist_offset) >= m_cheatCnt - 1) {
                m_cheat_index = 0;
                m_cheatlist_offset = 0;
                if (m_cheats[m_cheat_index + m_cheatlist_offset].enabled) return true;
            };
            do {
                if ((m_cheat_index < NUM_cheats - 1) && ((m_cheat_index + m_cheatlist_offset) < m_cheatCnt - 1))
                    m_cheat_index++;
                else if ((m_cheat_index + m_cheatlist_offset) < m_cheatCnt - 1)
                    m_cheatlist_offset++;
                else
                    break;
            } while (!m_cheats[m_cheat_index + m_cheatlist_offset].enabled);
            return true;
        }
        if (keysDown & HidNpadButton_B) {
            if (!m_outline_mode){
                m_outline_mode = true;
                ShowALlCheats->setState(true);
                m_cheat_index = m_cheat_index_save;
                m_cheatlist_offset = m_cheatlist_offset_save;
                return true;
            }
            // CloseThreads();
            if (save_code_to_file) {
                savetoggles();
                dumpcodetofile();
                for (u8 i = 0; i < m_cheatCnt; i++) {
                    dmntchtRemoveCheat(m_cheats[i].cheat_id);
                };
                loadcheatsfromfile();
                refresh_cheats = true;
                m_outline_refresh = true;
                getcheats();
                loadtoggles();
                refresh_cheats = true;
            }
            save_code_to_file = false;
            if (save_breeze_toggle_to_file) save_breeze_toggle();
            save_breeze_toggle_to_file = false;
            if (save_breeze_action_to_file) save_breeze_action();
            save_breeze_action_to_file = false;
            m_cheatlist_offset_save = m_cheatlist_offset;
            dmntchtResumeCheatProcess();
            // cleanup_se_tools();
            tsl::goBack();
            return true;
        }
        if (m_show_outline && !m_cursor_on_bookmark) {
            if ((keysDown & HidNpadButton_AnyUp) || (keysHeld & HidNpadButton_StickRUp)) {
                if (m_outline_index > 0)
                    m_outline_index--;
                else if (m_AttributeDumpBookmark->size() > 0)
                    m_cursor_on_bookmark = true;
                return true;
            }
            if ((keysDown & HidNpadButton_AnyDown) || (keysHeld & HidNpadButton_StickRDown)) {
                if (m_outline_index < m_outline.size() - 1) m_outline_index++;
                return true;
            };
            if (keysDown & HidNpadButton_A) {
                m_cheatlist_offset = m_outline[m_outline_index].index;
                m_cheat_index = 0;
                m_show_outline = false;
                return true;
            }
            // if (keysDown & HidNpadButton_B) {
            //     m_show_outline = false;
            //     return true;
            // }
            return true;
        }
        if ((keysDown & HidNpadButton_A) && m_cursor_on_bookmark) {
            valueStr = _getAddressDisplayString(m_selected_address, m_debugger, m_selected_type);
            value_pos = valueStr.length();
            m_edit_value = true;
            return true;
        }
        if ((keysDown & HidNpadButton_Plus) && !(keysHeld & HidNpadButton_ZL) && !m_cursor_on_bookmark) {
            addbookmark();
            return true;
        }
        if ((keysDown & HidNpadButton_Minus) && m_cursor_on_bookmark) {
            deletebookmark();
            if (((m_index >= NUM_bookmark) || ((m_index + m_addresslist_offset) >= (m_AttributeDumpBookmark->size() / sizeof(bookmark_t) ))) && m_index > 0) m_index--;
            return true;
        }
        if ((keysDown & HidNpadButton_Minus) && !m_cursor_on_bookmark && m_outline_mode) {
            auto outline_entry = m_cheat_outline[m_cheat_index + m_cheatlist_offset];
            if (outline_entry.is_outline && !outline_entry.always_expanded) {
                for (auto i = outline_entry.index; i <= outline_entry.index + outline_entry.size; i++)
                    dmntchtRemoveCheat(m_cheats[i].cheat_id);
            } else
                dmntchtRemoveCheat(m_cheats[outline_entry.index].cheat_id);
            m_outline_refresh = true;
            refresh_cheats = true;
            getcheats();
            save_code_to_file = true;
            m_cheatlist_offset = 0;
            m_cheat_index = 0;
            while (m_cheat_index + m_cheatlist_offset + 1 < m_cheat_outline.size()) {
                if (m_cheat_outline[m_cheat_index + m_cheatlist_offset + 1].index > redirect_index) break;
                if (m_cheat_index + 1 < m_NUM_cheats)
                    m_cheat_index++;
                else
                    m_cheatlist_offset++;
            }
            return true;
        };
        if ((keysDown & HidNpadButton_Minus) && !m_cursor_on_bookmark && !m_outline_mode) {
            dmntchtRemoveCheat(m_cheats[redirect_index].cheat_id);
            m_outline_refresh = true;
            refresh_cheats = true;
            getcheats();
            save_code_to_file = true;
            m_cheatlist_offset_save = 0;
            m_cheat_index_save = 0;

            while (m_cheat_index_save + m_cheatlist_offset_save + 1 < m_cheat_outline.size()) {
                if (m_cheat_outline[m_cheat_index_save + m_cheatlist_offset_save + 1].index > redirect_index) break;
                if (m_cheat_index_save + 1 < m_NUM_cheats)
                    m_cheat_index_save++;
                else
                    m_cheatlist_offset_save++;
            }
            return true;
        };
        // need update
        if (keysDown & HidNpadButton_StickR && !(keysHeld & HidNpadButton_ZL) && !m_cursor_on_bookmark) {  // programe key combo
            if (redirect_index == 0 || m_cheats[redirect_index].definition.opcodes[0] == 0x20000000) return true;
            keycode = 0x80000000;
            keycount = NUM_combokey;
            m_editCheat = true;
            save_code_to_file = true;
            return true;
        }
        if (keysDown & HidNpadButton_StickR && keysHeld & HidNpadButton_ZL && !m_cursor_on_bookmark) { // remove key combo
            if (redirect_index == 0 || m_cheats[redirect_index].definition.opcodes[0] == 0x20000000) return true;
            if ((m_cheats[redirect_index].definition.opcodes[0] & 0xF0000000) == 0x80000000 && (m_cheats[redirect_index].definition.opcodes[m_cheats[redirect_index].definition.num_opcodes - 1] & 0xF0000000) == 0x20000000) {
                for (u32 i = 0; i < m_cheats[redirect_index].definition.num_opcodes - 1; i++) {
                    m_cheats[redirect_index].definition.opcodes[i] = m_cheats[redirect_index].definition.opcodes[i + 1];
                };
                m_cheats[redirect_index].definition.num_opcodes -= 2;
                dmntchtRemoveCheat(m_cheats[redirect_index].cheat_id);
                u32 outid = 0;
                dmntchtAddCheat(&(m_cheats[redirect_index].definition), m_cheats[redirect_index].enabled, &outid);
                refresh_cheats = true;
            }
            save_code_to_file = true;
            return true;
        }
        if (keysDown & HidNpadButton_StickL && !(keysHeld & HidNpadButton_ZL)) {  // programe Breeze toggle key combo
            if (redirect_index == 0 || m_cheats[redirect_index].definition.opcodes[0] == 0x20000000) return true;
            if (m_cursor_on_bookmark) return true;
            keycode = 0x00000000;
            keycount = NUM_combokey;
            m_editCheat = true;
            if (m_cursor_on_bookmark) {
                // return true; // disable this option for now
                m_get_action_keycode = true;
                save_breeze_action_to_file = true;
            } else {
                m_get_toggle_keycode = true;
                save_breeze_toggle_to_file = true;
            }
            return true;
        }
        if (keysDown & HidNpadButton_StickL && keysHeld & HidNpadButton_ZL) {  // remove Breeze toggle key combo
            if (redirect_index == 0 || m_cheats[redirect_index].definition.opcodes[0] == 0x20000000) return true;
            //delete m_cheats[m_cheat_index + m_cheatlist_offset].cheat_id from m_toggle_list
            if (m_cursor_on_bookmark) {
                for (size_t i = 0; i < m_breeze_action_list.size(); i++) {
                    while ((m_breeze_action_list[i].index == m_index) &&  i < m_breeze_action_list.size())  {
                        m_breeze_action_list.erase(m_breeze_action_list.begin() + i);
                    }
                }
                save_breeze_action_to_file = true;
            } else {
                for (size_t i = 0; i < m_toggle_list.size(); i++) {
                    while (m_toggle_list[i].cheat_id == m_cheats[redirect_index].cheat_id && i < m_toggle_list.size()) {
                        m_toggle_list.erase(m_toggle_list.begin() + i);
                    }
                }
                save_breeze_toggle_to_file = true;
            }
            return true;
        }
        if ((keysHeld & HidNpadButton_StickL) && (keysHeld & HidNpadButton_StickR)) {
            // CloseThreads();
            // cleanup_se_tools();
            tsl::goBack();
            return true;
        };
        if ((keysDown & HidNpadButton_AnyUp) || (keysHeld & HidNpadButton_StickRUp)) {
            if (m_cursor_on_bookmark) {
                if (m_index > 0) m_index--;
            } else {
                m_cheat_index = std::min(m_cheat_index, m_NUM_cheats);
                if (m_outline.size() > 1 && m_outline[m_outline_index].index + m_cheats[0].cheat_id == m_cheats[m_cheat_index + m_cheatlist_offset].cheat_id) {  // && m_outline_index > 0) {
                    // m_cheatlist_offset = m_outline[m_outline_index].index;
                    // m_cheat_index = 0;
                    if (ShowALlCheats->getState()){
                        if (m_outline_index > 0)
                            m_outline_index--;
                    } else
                        return true;
                }
                if (m_cheat_index > 0)
                    m_cheat_index--;
                else {
                    if (m_cheatlist_offset > 0)
                        m_cheatlist_offset--;
                    else
                        if (m_AttributeDumpBookmark->size() > 0) m_cursor_on_bookmark = true;
                }
            }
            return true;
        };
        if ((keysDown & HidNpadButton_AnyDown) || (keysHeld & HidNpadButton_StickRDown)) {
            u32 m_cheatlineCnt = (m_outline_mode) ? m_cheat_outline.size() : m_cheatCnt;
            if (m_cursor_on_bookmark) {
                if ((m_index < NUM_bookmark - 1) && ((m_index + m_addresslist_offset) < (m_AttributeDumpBookmark->size() / sizeof(bookmark_t) - 1))) m_index++;
                else if (m_cheatlineCnt > 0)
                    m_cursor_on_bookmark = false;
            } else {
                m_cheat_index = std::min(m_cheat_index, (u32)(m_NUM_cheats - (u32)1));
                if (m_outline.size() > 1 && m_outline_index < (m_outline.size() - 1) && m_outline[m_outline_index + 1].index == m_cheats[m_cheat_index + m_cheatlist_offset].cheat_id + 1 - m_cheats[0].cheat_id) {
                    // m_cheatlist_offset = m_outline[m_outline_index].index;
                    // m_cheat_index = 0;
                    if (ShowALlCheats->getState()) {
                        if (m_outline_index < m_outline.size() - 1)
                            m_outline_index++;
                    } else
                        return true;
                }
                if ((m_cheat_index < NUM_cheats - 1) && ((m_cheat_index + m_cheatlist_offset) < m_cheatlineCnt - 1)) m_cheat_index++;
                else if ((m_cheat_index + m_cheatlist_offset) < m_cheatlineCnt - 1)
                    m_cheatlist_offset++;
            }
            return true;
        };
        if (((keysDown & HidNpadButton_L) || (keysDown & HidNpadButton_R)) && m_cursor_on_bookmark) {
            bookmark_t bookmark;
            m_AttributeDumpBookmark->getData((m_index + m_addresslist_offset) * sizeof(bookmark_t), &bookmark, sizeof(bookmark_t));
			if (bookmark.magic!=0x1289) bookmark.multiplier = 1;
            if (keysDown & HidNpadButton_R) {
                switch (bookmark.multiplier) {
                    case 1:
                        bookmark.multiplier = 2;
                        break;
                    case 2:
                        bookmark.multiplier = 4;
                        break;
                    case 4:
                        bookmark.multiplier = 8;
                        break;
                    case 8:
                        bookmark.multiplier = 16;
                        break;
                    case 16:
                        bookmark.multiplier = 32;
                        break;
                    case 32:
                        break;
                    default:
                        bookmark.multiplier = 1;
                        break;
                };
            } else {
                switch (bookmark.multiplier) {
                    case 32:
                        bookmark.multiplier = 16;
                        break;
                    case 16:
                        bookmark.multiplier = 8;
                        break;
                    case 8:
                        bookmark.multiplier = 4;
                        break;
                    case 4:
                        bookmark.multiplier = 2;
                        break;
                    default:
                        bookmark.multiplier = 1;
                        break;
                };
            };
			bookmark.magic = 0x1289;
            m_AttributeDumpBookmark->putData((m_index + m_addresslist_offset) * sizeof(bookmark_t), &bookmark, sizeof(bookmark_t));
            return true;
        };
        if ((keysDown & HidNpadButton_A) && !m_cursor_on_bookmark) { //*** toggle cheats / expand outline
            u32 index = m_cheat_index + m_cheatlist_offset;
            if (m_outline_mode) {
                auto entry = m_cheat_outline[index];
                if (entry.is_outline && !entry.always_expanded) {
                        if (!entry.expanded) {
                            for (u32 i = 1; i <= entry.size; i++) {
                                cheat_outline_entry_t new_entry = {};
                                new_entry.index = entry.index + i;
                                m_cheat_outline.insert(m_cheat_outline.begin() + index + i, new_entry);
                            }
                            entry.expanded = true;
                        } else {
                            m_cheat_outline.erase(m_cheat_outline.begin() + index + 1, m_cheat_outline.begin() + index + 1 + entry.size);
                            entry.expanded = false;
                        };
                        m_cheat_outline[index] = entry;
                        refresh_cheats = true;
                        return true;
                }
                index = entry.index;
            } else if (m_cheats[index].definition.opcodes[0] == 0x20000000) {
                if (m_outline.size() > 1)
                    m_show_outline = true;
                return true;
            }
            if (m_cheats[index].enabled)
                dmntchtToggleCheat(m_cheats[index].cheat_id);
            else {
                if (m_cheats[index].definition.num_opcodes + total_opcode <= MaximumProgramOpcodeCount)
                    dmntchtToggleCheat(m_cheats[index].cheat_id);
            }
            refresh_cheats = true;
            return true;
        }
        if (keysDown & HidNpadButton_R && (keysHeld & HidNpadButton_ZR)) {  // Next label
            if (m_outline_mode) {
                auto i = m_cheat_index + m_cheatlist_offset;
                while (i < m_cheat_outline.size()) {
                    i++;
                    if (m_cheat_outline[i].is_outline) {
                        m_cheatlist_offset = i - m_cheat_index;
                        break;
                    }
                }
            }
            return true;
        }
        if (keysDown & HidNpadButton_L && (keysHeld & HidNpadButton_ZR)) {  // Previous label
            if (m_outline_mode) {
                auto i = m_cheat_index + m_cheatlist_offset;
                while (i > 0) {
                    i--;
                    if (m_cheat_outline[i].is_outline) {
                        if (i >= m_cheat_index)
                            m_cheatlist_offset = i - m_cheat_index;
                        else {
                            m_cheatlist_offset = 0;
                            m_cheat_index = i;
                        }
                        break;
                    }
                }
            }
            return true;
        }
        if (keysDown & HidNpadButton_R && !(keysHeld & HidNpadButton_ZR)) {  //page down
            if (m_outline_mode) {
                if ((m_cheatlist_offset + NUM_cheats) < m_cheat_outline.size() - 1) m_cheatlist_offset += NUM_cheats;
                if ((m_cheat_index + m_cheatlist_offset) > m_cheat_outline.size() - 1) m_cheat_index = m_cheat_outline.size() - 1 - m_cheatlist_offset;
                return true;
            }
            if (!m_cursor_on_bookmark && (m_outline.size() <= 1 || ShowALlCheats->getState())) {
                if ((m_cheatlist_offset + NUM_cheats) < m_cheatCnt - 1) m_cheatlist_offset += NUM_cheats;
                if ((m_cheat_index + m_cheatlist_offset) > m_cheatCnt - 1) m_cheat_index = m_cheatCnt - 1 - m_cheatlist_offset;
                //update m_outline_index
                size_t i = 0;
                while (i < m_outline.size() && (m_cheat_index + m_cheatlist_offset) > m_outline[i].index) {
                    m_outline_index = i;
                    i++;
                }

            } else if (!m_cursor_on_bookmark && m_outline.size() > 1) {  // show within outline only case
                if (m_outline_index < m_outline.size() - 1) m_outline_index++;
                m_cheatlist_offset = m_outline[m_outline_index].index;
                m_cheat_index = 0;
            };
            return true;
        }
        if (keysDown & HidNpadButton_L && !(keysHeld & HidNpadButton_ZR)) {  //page up
            if (!m_cursor_on_bookmark && (m_outline.size() <= 1 || ShowALlCheats->getState())) {
                if (m_cheatlist_offset > NUM_cheats)
                    m_cheatlist_offset -= NUM_cheats;
                else
                    m_cheatlist_offset = 0;
                //update m_outline_index
                size_t i = 0;
                while (i < m_outline.size() && (m_cheat_index + m_cheatlist_offset) > m_outline[i].index) {
                    m_outline_index = i;
                    i++;
                }

            } else if (!m_cursor_on_bookmark && m_outline.size() > 1) {  // show within outline only case
                if (m_outline_index > 0) m_outline_index--;
                m_cheatlist_offset = m_outline[m_outline_index].index;
                m_cheat_index = 0;
            };
            return true;
        }

        return false;
    }
};
class PickCheatsOverlay : public tsl::Gui {
   public:
    u32 cache_outline_index = 0, cache_outline_offset = 0;
    u32 Selected_count = 0, cheat_slot_available = 0;
    bool cheats_added = false;
    PickCheatsOverlay() {
        cheats_added = false;
        if (loadcachefromfile()) {
            u32 index;
            if (m_cache_outline.size() > 0)
                index = m_cache_outline[0].index;
            else 
                index = m_cache.size();
            for (u32 i = 0; i < index; i++) {
                cache_outline_t new_entry = {};
                new_entry.index = i;
                m_cache_outline.insert(m_cache_outline.begin() + i, new_entry);
            }
        } 

        // insert the cheats the comes before the first label

    }
    virtual tsl::elm::Element *createUI() override {
        auto rootFrame = new tsl::elm::OverlayFrame("", "");
        auto Status = new tsl::elm::CustomDrawer([](tsl::gfx::Renderer *renderer, u16 x, u16 y, u16 w, u16 h) {
            renderer->drawRect(0, 0, tsl::cfg::FramebufferWidth , tsl::cfg::FramebufferHeight, a(0x7111));
            renderer->m_maxY = 0;
            renderer->drawString(CheatsLabelsStr, false, 65, fontsize, fontsize, renderer->a(0xFFFF));
            renderer->drawString(CheatsCursor, false, 5, fontsize, fontsize, renderer->a(0xFFFF));
            renderer->drawString(CheatsEnableStr, false, 25, fontsize, fontsize, renderer->a(0xFFFF));
            m_NUM_cheats = std::min(m_displayed_cheat_lines + (s32)(tsl::cfg::FramebufferHeight - renderer->m_maxY) / (fontsize + 3), (u32)MAX_NUM_cheats);
        });
        rootFrame->setContent(Status);
        return rootFrame;
    }
    virtual void update() override {
        char ss[200] = "";
        m_displayed_cheat_lines = 0;
        CheatsLabelsStr[0] = 0;
        CheatsCursor[0] = 0;
        CheatsEnableStr[0] = 0;
        snprintf(CheatsLabelsStr, sizeof CheatsLabelsStr, "\n\n");
        snprintf(CheatsEnableStr, sizeof CheatsEnableStr, "\n\n");
        Selected_count = 0;
        for (u8 i = 0; i < m_cache_outline.size(); i++)
            if (m_cache_outline[i].selected) Selected_count++;
        if (m_cheatCnt > 0)
            cheat_slot_available = MaxCheatCount - m_cheats[0].cheat_id - m_cheatCnt;
        else
            cheat_slot_available = MaxCheatCount - 1;
        snprintf(CheatsCursor, sizeof CheatsCursor, "Cheat index = %d, Total selected = %d/%d, Total cheats in file %ld\n\uE0A0 Select, \uE0A2 Add Cheats, \uE0A7+\uE0A4\uE0A5 Jump to label\n", m_cache_outline[cache_outline_index + cache_outline_offset].index, Selected_count, cheat_slot_available, m_cache.size()); // cache_outline_index, cache_outline_offset, m_NUM_cheats);
        for (u8 line = 0; line < NUM_cheats; line++) {
            if ((line + cache_outline_offset) >= m_cache_outline.size())
                break;
            {
                DmntCheatEntry cheat_entry = m_cache[m_cache_outline[line + cache_outline_offset].index];
                auto entry = m_cache_outline[line + cache_outline_offset];
                if (cheat_entry.definition.opcodes[0] == 0x20000000) {
                    snprintf(ss, sizeof ss, "[%s] %d\n", cheat_entry.definition.readable_name, m_cache_outline[line + cache_outline_offset].size);
                    strcat(CheatsLabelsStr, ss);
                    // strcat(CheatsEnableStr, ss);
                } else if (cheat_entry.definition.opcodes[0] == 0x20000001) {
                    snprintf(ss, sizeof ss, "- outline off - [%s]\n", cheat_entry.definition.readable_name);
                    strcat(CheatsLabelsStr, ss);
                    // strcat(CheatsEnableStr, ss);
                } else {
                    snprintf(ss, sizeof ss, "%s\n", cheat_entry.definition.readable_name);
                    strcat(CheatsLabelsStr, ss);

                }
                snprintf(ss, sizeof ss, "%s\n", (entry.selected) ? "\u25A0" : "\u25A1");
                strcat(CheatsEnableStr, ss);
                snprintf(ss, sizeof ss, "%s\n", ((cache_outline_index == line) ? "\uE019" : ""));
                strcat(CheatsCursor, ss);
                m_displayed_cheat_lines++;
            }
        }
    }
    virtual bool handleInput(u64 keysDown, u64 keysHeld, HidTouchState touchInput, JoystickPosition leftJoyStick, JoystickPosition rightJoyStick) override {
        if (keysDown & HidNpadButton_B) {
            if (cheats_added) {
                refresh_cheats = true;
                m_outline_refresh = true;
                getcheats();
                savetoggles();
                dumpcodetofile();
                for (u8 i = 0; i < m_cheatCnt; i++) {
                    dmntchtRemoveCheat(m_cheats[i].cheat_id);
                };
                loadcheatsfromfile();
                loadtoggles();
            };
            m_outline_refresh = true;
            refresh_cheats = true;
            m_outline_mode = true;
            m_cheat_index = 0;
            m_cheatlist_offset = 0;
            tsl::goBack();
            return true;
        }
        if (keysDown & HidNpadButton_Minus) {
            // Clear existing cheats is desired
            refresh_cheats = true;
            getcheats();
            savetoggles();
            for (u8 i = 0; i < m_cheatCnt; i++) {
                dmntchtRemoveCheat(m_cheats[i].cheat_id);
            };
            m_outline_refresh = true;
            refresh_cheats = true;
            getcheats();
            return true;
        }
        if (keysDown & HidNpadButton_X) {
            // proceed to add selected cheats
            u32 idout;
            for (auto entry : m_cache_outline) {
                if (entry.selected) {
                    cheats_added = true;
                    if (m_cache[entry.index].enabled)
                        dmntchtSetMasterCheat(&(m_cache[entry.index].definition));
                    else
                        dmntchtAddCheat(&(m_cache[entry.index].definition), false, &idout);
                }
            }
            refresh_cheats = true;
            m_outline_refresh = true;
            getcheats();
            loadtoggles();
            refresh_cheats = true;
            return true;
        }
        if (keysDown & HidNpadButton_A) { //*** toggle cheats / expand outline
            u32 index = cache_outline_index + cache_outline_offset;
            {
                auto entry = m_cache_outline[index];
                if (entry.is_outline && !entry.always_expanded) {
                    if (entry.selected) {
                        entry.selected = false;
                        m_cache_outline[index] = entry;
                        return true;
                    }
                    if (!entry.expanded) {
                        for (u32 i = 1; i <= entry.size; i++) {
                            cache_outline_t new_entry = {};
                            new_entry.index = entry.index + i;
                            if (entry.size + Selected_count < cheat_slot_available)
                                new_entry.selected = true;
                            m_cache_outline.insert(m_cache_outline.begin() + index + i, new_entry);
                        }
                        entry.expanded = true;
                        if (Selected_count < cheat_slot_available)
                            entry.selected = true;
                    } else {
                        m_cache_outline.erase(m_cache_outline.begin() + index + 1, m_cache_outline.begin() + index + 1 + entry.size);
                        entry.expanded = false;
                        entry.selected = false;
                    };
                    // entry.selected = true;
                    m_cache_outline[index] = entry;
                    // refresh_cheats = true;
                    return true;
                }
                // index = entry.index;
            }
            if (m_cache_outline[index].selected)
                m_cache_outline[index].selected = false;
            else {
                if (Selected_count < cheat_slot_available)
                    m_cache_outline[index].selected = true;
            }
            // refresh_cheats = true;
            return true;
        }
        if (keysDown & HidNpadButton_R && keysHeld & HidNpadButton_ZL) {
            fontsize++;
            return true;
        }
        if (keysDown & HidNpadButton_L && keysHeld & HidNpadButton_ZL) {
            fontsize--;
            return true;
        }
        if ((keysDown & HidNpadButton_AnyUp) || (keysHeld & HidNpadButton_StickRUp)) {
            {
                cache_outline_index = std::min(cache_outline_index, m_NUM_cheats);
                if (cache_outline_index > 0)
                    cache_outline_index--;
                else {
                    if (cache_outline_offset > 0)
                        cache_outline_offset--;
                }
            }
            return true;
        };
        if ((keysDown & HidNpadButton_AnyDown) || (keysHeld & HidNpadButton_StickRDown)) {
            {
                cache_outline_index = std::min(cache_outline_index, (u32)(m_NUM_cheats - (u32)1));
                if ((cache_outline_index < NUM_cheats - 1) && ((cache_outline_index + cache_outline_offset) < m_cache_outline.size() - 1))
                    cache_outline_index++;
                else if ((cache_outline_index + cache_outline_offset) < m_cache_outline.size() - 1)
                    cache_outline_offset++;
            }
            return true;
        };

        if (keysDown & HidNpadButton_R && (keysHeld & HidNpadButton_ZR)) {  // Next label
             {
                size_t i = cache_outline_index + cache_outline_offset;
                while (i < m_cache_outline.size()) {
                    i++;
                    if (m_cache_outline[i].is_outline) {
                        cache_outline_offset = i - cache_outline_index;
                        break;
                    }
                }
            }
            return true;
        }
        if (keysDown & HidNpadButton_L && (keysHeld & HidNpadButton_ZR)) {  // Previous label
             {
                auto i = cache_outline_index + cache_outline_offset;
                while (i > 0) {
                    i--;
                    if (m_cache_outline[i].is_outline) {
                        if (i >= cache_outline_index)
                            cache_outline_offset = i - cache_outline_index;
                        else {
                            cache_outline_offset = 0;
                            cache_outline_index = i;
                        }
                        break;
                    }
                }
            }
            return true;
        }
        if (keysDown & HidNpadButton_R && !(keysHeld & HidNpadButton_ZR)) {  //page down
             {
                if ((cache_outline_offset + NUM_cheats) < m_cache_outline.size() - 1) cache_outline_offset += NUM_cheats;
                if ((cache_outline_index + cache_outline_offset) > m_cache_outline.size() - 1) cache_outline_index = m_cache_outline.size() - 1 - cache_outline_offset;
                return true;
            }
            return true;
        }
        if (keysDown & HidNpadButton_L && !(keysHeld & HidNpadButton_ZR)) {  //page up
            if (cache_outline_offset > NUM_cheats)
                cache_outline_offset -= NUM_cheats;
            else
                cache_outline_offset = 0;
            return true;
        }
        return false;
    }
};
//Main Menu
class MainMenu : public tsl::Gui { // WIP
   public:
    MainMenu() {}

    virtual tsl::elm::Element *createUI() override {
        auto rootFrame = new tsl::elm::OverlayFrame("Zing", APP_VERSION);
        auto list = new tsl::elm::List();
        if (m_debugger->m_dmnt) {
            auto Bookmark = new tsl::elm::ListItem("Activate Monitor");
            Bookmark->setClickListener([](uint64_t keys) {
                if (keys & HidNpadButton_A) {
                    // StartThreads();
                    // if (!init_se_tools()) return true;
                    TeslaFPS = 20;
                    refreshrate = 1;
                    alphabackground = 0x0;
                    tsl::hlp::requestForeground(false);
                    FullMode = false;
                    tsl::changeTo<BookmarkOverlay>();
                    return true;
                }
                return false;
            });
            list->addItem(Bookmark);

            auto SetMultiplier = new tsl::elm::ListItem("Settings");
            SetMultiplier->setClickListener([](uint64_t keys) {
                if (keys & HidNpadButton_A) {
                    // StartThreads();
                    // if (!init_se_tools()) return true;
                    TeslaFPS = 50;
                    refreshrate = 1;
                    alphabackground = 0x0;
                    tsl::hlp::requestForeground(true);
                    FullMode = false;
                    tsl::changeTo<SetMultiplierOverlay>();
                    return true;
                }
                return false;
            });
            list->addItem(SetMultiplier);

            auto PickCheats = new tsl::elm::ListItem("Pick Cheats from large list");
            PickCheats->setClickListener([](uint64_t keys) {
                if (keys & HidNpadButton_A) {
                    // StartThreads();
                    // if (!init_se_tools()) return true;
                    TeslaFPS = 50;
                    refreshrate = 1;
                    alphabackground = 0x0;
                    tsl::hlp::requestForeground(true);
                    FullMode = false;
                    tsl::changeTo<PickCheatsOverlay>();
                    return true;
                }
                return false;
            });
            list->addItem(PickCheats);

            auto LoadCheats = new tsl::elm::ListItem("reload Cheats");
            LoadCheats->setClickListener([](uint64_t keys) {
                if (keys & HidNpadButton_A) {
                    // init_se_tools();
                    refresh_cheats = true;
                    getcheats();
                    savetoggles();
                    for (u8 i = 0; i < m_cheatCnt; i++) {
                        dmntchtRemoveCheat(m_cheats[i].cheat_id);
                    };
                    loadcheatsfromfile();
                    refresh_cheats = true;
                    m_outline_refresh = true;
                    getcheats();
                    loadtoggles();
                    refresh_cheats = true;
                    // cleanup_se_tools();
                    return true;
                }
                return false;
            });
            list->addItem(LoadCheats);
            ShowALlCheats = new tsl::elm::ToggleListItem("Show All cheats", true);

            ShowKeyHints = new tsl::elm::ToggleListItem("Show key hints", false);

            // list->addItem(ShowALlCheats);

            list->addItem(ShowKeyHints);

        } else {
            auto NoCheats = new tsl::elm::ListItem("Dmnt not attached");
            list->addItem(NoCheats);
        }
        // auto MailBox = new tsl::elm::ListItem("MailBox");
        // MailBox->setClickListener([](uint64_t keys) {
        // 	if (keys & HidNpadButton_A) {
        // 		StartThreads();
        // 		TeslaFPS = 1;
        // 		refreshrate = 1;
        // 		alphabackground = 0x0;
        // 		tsl::hlp::requestForeground(false);
        // 		FullMode = false;
        // 		tsl::changeTo<MailBoxOverlay>();
        // 		return true;
        // 	}
        // 	return false;
        // });
        // list->addItem(MailBox);

        // auto Full = new tsl::elm::ListItem("Full");
        // Full->setClickListener([](uint64_t keys) {
        // 	if (keys & HidNpadButton_A) {
        // 		StartThreads();
        // 		TeslaFPS = 1;
        // 		refreshrate = 1;
        // 		tsl::hlp::requestForeground(false);
        // 		tsl::changeTo<FullOverlay>();
        // 		return true;
        // 	}
        // 	return false;
        // });
        // list->addItem(Full);

        // auto Mini = new tsl::elm::ListItem("Mini");
        // Mini->setClickListener([](uint64_t keys) {
        // 	if (keys & HidNpadButton_A) {
        // 		StartThreads();
        // 		TeslaFPS = 1;
        // 		refreshrate = 1;
        // 		alphabackground = 0x0;
        // 		tsl::hlp::requestForeground(false);
        // 		FullMode = false;
        // 		tsl::changeTo<MiniOverlay>();
        // 		return true;
        // 	}
        // 	return false;
        // });
        // list->addItem(Mini);

        if (SaltySD == true) {
            auto comFPS = new tsl::elm::ListItem("FPS Counter");
            comFPS->setClickListener([](uint64_t keys) {
                if (keys & HidNpadButton_A) {
                    StartFPSCounterThread();
                    TeslaFPS = 31;
                    refreshrate = 31;
                    alphabackground = 0x0;
                    tsl::hlp::requestForeground(false);
                    FullMode = false;
                    tsl::changeTo<com_FPS>();
                    return true;
                }
                return false;
            });
            list->addItem(comFPS);
        }
#ifdef CUSTOM
        auto Custom = new tsl::elm::ListItem("Custom");
        Custom->setClickListener([](uint64_t keys) {
            if (keys & HidNpadButton_A) {
                StartThreads();
                StartBatteryThread();
                TeslaFPS = 1;
                refreshrate = 1;
                tsl::hlp::requestForeground(false);
                tsl::changeTo<CustomOverlay>();
                return true;
            }
            return false;
        });
        list->addItem(Custom);
#endif

        rootFrame->setContent(list);

        return rootFrame;
    }

    virtual void update() override {
        if (TeslaFPS != 60) {
            FullMode = true;
            tsl::hlp::requestForeground(true);
            TeslaFPS = 60;
            alphabackground = 0xD;
            refreshrate = 1;
            systemtickfrequency = 19200000;
        }
        if (first_launch && m_debugger->m_dmnt) {
            // reload the cheats
            // refresh_cheats = true;
            // getcheats();
            // savetoggles();
            // for (u8 i = 0; i < m_cheatCnt; i++) {
            //     dmntchtRemoveCheat(m_cheats[i].cheat_id);
            // };
            // loadcheatsfromfile();
            refresh_cheats = true;
            getcheats();
            // loadtoggles();
            // refresh_cheats = true;
            // if (m_outline.size() > 1) {  // && m_AttributeDumpBookmark->size() == 0) {
            //     // m_show_outline = true;
            //     // m_cursor_on_bookmark = false;
            // }
            // end reload cheats

            first_launch = false;
            TeslaFPS = 50;
            refreshrate = 1;
            alphabackground = 0x0;
            tsl::hlp::requestForeground(true);
            FullMode = false;
            tsl::changeTo<SetMultiplierOverlay>();
        }
        // if (Bstate.A == 123) {
        //     tsl::hlp::requestForeground(true);
        //     Bstate.A += 100;
        // };
        // Bstate.B += 1;
    }
    virtual bool handleInput(u64 keysDown, u64 keysHeld, HidTouchState touchInput, JoystickPosition leftJoyStick, JoystickPosition rightJoyStick) override {
        if (keysDown & HidNpadButton_B) {
            dmntchtResumeCheatProcess();
            tsl::goBack();
            return true;
        }
        if (keysHeld & HidNpadButton_X) {
            TeslaFPS = 50;
            refreshrate = 1;
            alphabackground = 0x0;
            tsl::hlp::requestForeground(true);
            FullMode = false;
            tsl::changeTo<SetMultiplierOverlay>();
            return true;
        }
        return false;
    }
};

class MonitorOverlay : public tsl::Overlay {
   public:
    virtual void initServices() override {
        dmntchtCheck = dmntchtInitialize();
        rc = nsInitialize();
        init_se_tools();
        load_breeze_toggle();
        load_breeze_action();
        //Initialize services
        if (R_SUCCEEDED(smInitialize())) {
            if (hosversionAtLeast(8, 0, 0))
                clkrstCheck = clkrstInitialize();
            else
                pcvCheck = pcvInitialize();

            tsCheck = tsInitialize();
            if (hosversionAtLeast(5, 0, 0)) tcCheck = tcInitialize();

            if (R_SUCCEEDED(fanInitialize())) {
                if (hosversionAtLeast(7, 0, 0))
                    fanCheck = fanOpenController(&g_ICon, 0x3D000001);
                else
                    fanCheck = fanOpenController(&g_ICon, 1);
            }

            if (R_SUCCEEDED(nvInitialize())) nvCheck = nvOpen(&fd, "/dev/nvhost-ctrl-gpu");

#ifdef CUSTOM
            psmCheck = psmInitialize();
            if (R_SUCCEEDED(psmCheck)) psmService = psmGetServiceSession();
#endif

            // Atmosphere_present = isServiceRunning("dmnt:cht");
            // SaltySD = CheckPort();
            // if (SaltySD == true && Atmosphere_present == true) dmntchtCheck = dmntchtInitialize();

            if (SaltySD == true) {
                //Assign NX-FPS to default core
                threadCreate(&t6, CheckIfGameRunning, NULL, NULL, 0x1000, 0x38, -2);

                //Start NX-FPS detection
                threadStart(&t6);
            }
            smExit();
        }
        Hinted = envIsSyscallHinted(0x6F);
    }

    virtual void exitServices() override {
        CloseThreads();

        //Exit services
        cleanup_se_tools();
        svcCloseHandle(debug);
        dmntchtExit();
        nsExit();
        clkrstExit();
        pcvExit();
        tsExit();
        tcExit();
        fanControllerClose(&g_ICon);
        fanExit();
        nvClose(fd);
        nvExit();
#ifdef CUSTOM
        psmExit();
#endif
    }

    virtual void onShow() override {
        m_on_show = true;
        refresh_cheats = true;
        if (m_debugger->m_dmnt)
            CloseThreads();
    }  // Called before overlay wants to change from invisible to visible state
    virtual void onHide() override {
        if (m_debugger->m_dmnt)
            StartThreads();
    }  // Called before overlay wants to change from visible to invisible state

    virtual std::unique_ptr<tsl::Gui> loadInitialGui() override {
        return initially<MainMenu>();  // Initial Gui to load. It's possible to pass arguments to it's constructor like this
    }
};

// This function gets called on startup to create a new Overlay object
int main(int argc, char **argv) {
    return tsl::loop<MonitorOverlay>(argc, argv);
}
