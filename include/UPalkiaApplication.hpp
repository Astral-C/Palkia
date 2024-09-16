#pragma once

#include "UApplication.hpp"

class UPalkiaApplication : public UApplication {
	struct GLFWwindow* mWindow;
	class UPalkiaContext* mContext;

	virtual bool Execute(float deltaTime) override;

	
public:

	UPalkiaApplication();
	virtual ~UPalkiaApplication() {}

	virtual bool Setup() override;
	virtual bool Teardown() override;
};