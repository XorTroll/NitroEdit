#include <base_SelfNitroFs.hpp>
#include <ui/menu/menu_MainMenu.hpp>
#include <ntr/fs/fs_Stdio.hpp>

#define STB_TRUETYPE_IMPLEMENTATION
#include <extra/stb_truetype.h>

namespace {

	bool g_Initialized = false;

	void ATTR_NORETURN OnCriticalErrorNoUi(const std::string &err) {
		consoleDemoInit();
		printf("\n\n");
		printf("-] NitroEdit - critical error [-");
		printf("\n");
		printf("--------------------------------");
		printf("\n");
		printf("%s\n", err.c_str());
		printf("\n");
		printf("--------------------------------");
		printf("\n");
		printf("-] Please restart this app... [-");
		while(true) {
			swiWaitForVBlank();
		}
		__builtin_unreachable();
	}

	constexpr auto DefaultSelfPath = "NitroEdit.nds";

	static inline const std::string RootDirectory = "NitroEdit";
	static inline const std::string ExternalFsDirectory = RootDirectory + "/ext_fs";

	void Initialize(int argc, const char *argv[]) {
		defaultExceptionHandler();
		srand(time(nullptr));

		ASSERT_TRUE(fatInitDefault());
		ntr::fs::SetExternalFsDirectory(ExternalFsDirectory);
		ntr::fs::DeleteStdioDirectory(ExternalFsDirectory);

		if(argc > 0) {
			ASSERT_TRUE(InitializeSelfNitroFs(argv[0]) || InitializeSelfNitroFs(DefaultSelfPath));
		}
		else {
			ASSERT_TRUE(InitializeSelfNitroFs(DefaultSelfPath));
		}

		ui::menu::Initialize();

		g_Initialized = true;
	}

}

void ATTR_NORETURN OnCriticalError(const std::string &err) {
	if(!g_Initialized) {
		OnCriticalErrorNoUi(err);
	}
	else {
		// Set this to false, so that if ShowDialogLoop() happens to fail, it'll show the error on console to avoid an infinite loop of calls
		g_Initialized = false;
		ui::menu::ShowDialogLoop("A critical error ocurred:\n\n" + err + "\n\nPlease restart this app");
		while(true);
		__builtin_unreachable();
	}
}

int main(int argc, const char *argv[]) {
	Initialize(argc, argv);

	ui::menu::LoadMainMenu();

	ui::menu::MainLoop();

	return 0;
}