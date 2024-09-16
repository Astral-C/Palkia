#include <UPalkiaApplication.hpp>

int main(int argc, char* argv[]) {
	UPalkiaApplication app;

	if (!app.Setup()) {
		return 0;
	}

	app.Run();

	if (!app.Teardown()) {
		return 0;
	}
}