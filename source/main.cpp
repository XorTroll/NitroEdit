#include <ntr/ntr_ROM.hpp>
#include <ntr/ntr_NARC.hpp>
#include <ntr/ntr_BMG.hpp>
#include <ntr/ntr_NCGR.hpp>
#include <ntr/ntr_NCLR.hpp>
#include <ntr/ntr_NSCR.hpp>
#include <gfx/gfx_Conversion.hpp>
#include <gfx/gfx_BannerIcon.hpp>
#include <fs/fs_BinaryFile.hpp>
#include <fs/fs_Fat.hpp>
#include <ntr/nfs/nfs_SelfNitroFs.hpp>
#include <util/util_String.hpp>

#include <ui/menu/menu_Base.hpp>
#include <ui/menu/menu_MainMenu.hpp>

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

	void OnException() {
		OnCriticalError("An exception ocurred (likely a data abort)");
	}

	constexpr auto DefaultSelfPath = "NitroEdit.nds";

	void Initialize(int argc, const char *argv[]) {
		setExceptionHandler(OnException);
		srand(time(nullptr));

		ASSERT_TRUE(fs::InitializeFat());

		if(argc > 0) {
			ASSERT_TRUE(ntr::nfs::InitializeSelfNitroFs(argv[0]) || ntr::nfs::InitializeSelfNitroFs(DefaultSelfPath));
		}
		else {
			ASSERT_TRUE(ntr::nfs::InitializeSelfNitroFs(DefaultSelfPath));
		}

		fs::DeleteFatDirectory(ExternalFsDirectory);

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