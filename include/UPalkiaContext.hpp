#pragma once

#include "UCamera.hpp"
#include "Models/NSBMD.hpp"

#include <map>
#include <vector>
#include <filesystem>
#include <memory>

class UPalkiaContext {

	uint32_t mGizmoOperation { 0 };

	std::vector<std::pair<uint32_t, glm::mat4>> mObjects;
	std::map<uint32_t, Palkia::NSBMD> mModels;
	Palkia::NSBMD mMapModel;

	USceneCamera mCamera;

	uint32_t mMainDockSpaceID;
	uint32_t mDockNodeLeftID;
	uint32_t mDockNodeUpLeftID;
	uint32_t mDockNodeDownLeftID;
	
	bool mOptionsOpen { false };
	bool mViewportIsFocused { false };

	bool bIsDockingSetUp { false };
	bool bIsFileDialogOpen { false };
	bool bIsSaveDialogOpen { false };

	// Rendering surface
	uint32_t mFbo, mRbo, mViewTex, mPickTex;

	float mPrevWinWidth { -1.0f };
	float mPrevWinHeight { -1.0f };

	uint32_t mLoadMap { 0 };

	void RenderMainWindow(float deltaTime);
	void RenderPanels(float deltaTime);
	void RenderMenuBar();

public:
	UPalkiaContext();
	~UPalkiaContext();


	void HandleSelect();
	bool Update(float deltaTime);
	void Render(float deltaTime);
	USceneCamera* GetCamera() { return &mCamera; }
};
