#define BSTREAM_IMPLEMENTATION
#include <bstream.h>

#include <UPalkiaContext.hpp>

#include <ImGuiFileDialog/ImGuiFileDialog.h>
#include <glad/glad.h>
#include <imgui.h>
#include <imgui_internal.h>
#include "ImGuizmo.h"

#include <System/Rom.hpp>
#include <System/Archive.hpp>
#include <Models/NSBMD.hpp>
#include <Util.hpp>

#include "IconsForkAwesome.h"


UPalkiaContext::~UPalkiaContext(){
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	glDeleteFramebuffers(1, &mFbo);
	glDeleteRenderbuffers(1, &mRbo);
	glDeleteTextures(1, &mViewTex);
	glDeleteTextures(1, &mPickTex);
}

UPalkiaContext::UPalkiaContext(){

	//Palkia::Nitro::Rom Platinum(std::filesystem::path("platinum.nds"));

	ImGuiIO& io = ImGui::GetIO();
	
	if(std::filesystem::exists((std::filesystem::current_path() / "res" / "NotoSansJP-Regular.otf"))){
		io.Fonts->AddFontFromFileTTF((std::filesystem::current_path() / "res" / "NotoSansJP-Regular.otf").string().c_str(), 16.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
	}

	if(std::filesystem::exists((std::filesystem::current_path() / "res" / "forkawesome.ttf"))){
		static const ImWchar icons_ranges[] = { ICON_MIN_FK, ICON_MAX_16_FK, 0 };
		ImFontConfig icons_config; 
		icons_config.MergeMode = true; 
		icons_config.PixelSnapH = true; 
		icons_config.GlyphMinAdvanceX = 16.0f;
		io.Fonts->AddFontFromFileTTF((std::filesystem::current_path() / "res" / "forkawesome.ttf").string().c_str(), icons_config.GlyphMinAdvanceX, &icons_config, icons_ranges );
	}
	
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	mGizmoOperation = ImGuizmo::OPERATION::TRANSLATE;
}

bool UPalkiaContext::Update(float deltaTime) {
	if(mViewportIsFocused){
		mCamera.Update(deltaTime);

		if(ImGui::IsKeyPressed(ImGuiKey_1)){
			mGizmoOperation = ImGuizmo::OPERATION::TRANSLATE;
		}

		if(ImGui::IsKeyPressed(ImGuiKey_2)){
			mGizmoOperation = ImGuizmo::OPERATION::ROTATE;
		}

		if(ImGui::IsKeyPressed(ImGuiKey_3)){
			mGizmoOperation = ImGuizmo::OPERATION::SCALE;
		}

	}

	return true;
}

void UPalkiaContext::Render(float deltaTime) {
	ImGuiIO& io = ImGui::GetIO();

	RenderMenuBar();
	
	const ImGuiViewport* mainViewport = ImGui::GetMainViewport();

	ImGuiDockNodeFlags dockFlags = ImGuiDockNodeFlags_PassthruCentralNode | ImGuiDockNodeFlags_AutoHideTabBar | ImGuiDockNodeFlags_NoDockingInCentralNode;
	mMainDockSpaceID = ImGui::DockSpaceOverViewport(0, mainViewport, dockFlags);
	
	if(!bIsDockingSetUp){

		glGenFramebuffers(1, &mFbo);
		glBindFramebuffer(GL_FRAMEBUFFER, mFbo);

		glGenRenderbuffers(1, &mRbo);
		glBindRenderbuffer(GL_RENDERBUFFER, mRbo);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, 1280, 720);

		glGenTextures(1, &mViewTex);
		glGenTextures(1, &mPickTex);

		glBindTexture(GL_TEXTURE_2D, mViewTex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1280, 720, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);

		glBindTexture(GL_TEXTURE_2D, mPickTex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_R32UI, 1280, 720, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, nullptr);

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mViewTex, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, mPickTex, 0);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, mRbo);

		assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

		ImGui::DockBuilderRemoveNode(mMainDockSpaceID); // clear any previous layout
		ImGui::DockBuilderAddNode(mMainDockSpaceID, dockFlags | ImGuiDockNodeFlags_DockSpace);
		ImGui::DockBuilderSetNodeSize(mMainDockSpaceID, mainViewport->Size);


		mDockNodeLeftID = ImGui::DockBuilderSplitNode(mMainDockSpaceID, ImGuiDir_Left, 0.20f, nullptr, &mMainDockSpaceID);
		mDockNodeDownLeftID = ImGui::DockBuilderSplitNode(mDockNodeLeftID, ImGuiDir_Up, 1.0f, nullptr, &mDockNodeUpLeftID);


		ImGui::DockBuilderDockWindow("mainWindow", mDockNodeDownLeftID);
		ImGui::DockBuilderDockWindow("viewportWindow", mMainDockSpaceID);

		ImGui::DockBuilderFinish(mMainDockSpaceID);
		bIsDockingSetUp = true;
	}


	ImGuiWindowClass mainWindowOverride;
	mainWindowOverride.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_NoTabBar;
	ImGui::SetNextWindowClass(&mainWindowOverride);
	
	ImGui::Begin("mainWindow", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);
	ImGui::Text("Map");
		ImGui::Separator();
		ImGui::InputInt("Map ID", &mLoadMap);
		if(ImGui::Button("Load")){
			
			Palkia::Nitro::Rom rom("platinum.nds");

			//rom.Dump();
			std::shared_ptr<Palkia::Nitro::File> buildModelFile = rom.GetFile("fielddata/build_model/build_model.narc");
			std::shared_ptr<Palkia::Nitro::File> landDataFile = rom.GetFile("fielddata/land_data/land_data.narc");
			
			bStream::CMemoryStream buildModelStream(buildModelFile->GetData(), buildModelFile->GetSize(), bStream::Endianess::Little, bStream::OpenMode::In);
			bStream::CMemoryStream landDataStream(landDataFile->GetData(), landDataFile->GetSize(), bStream::Endianess::Little, bStream::OpenMode::In);
			
			Palkia::Nitro::Archive buildModelArc(buildModelStream);
			Palkia::Nitro::Archive landDataArc(landDataStream);

			mMapModel = Palkia::NSBMD();
			mModels.clear();
			mObjects.clear();

			std::shared_ptr<Palkia::Nitro::File> mapFile = landDataArc.GetFileByIndex(mLoadMap);
			bStream::CMemoryStream map(mapFile->GetData(), mapFile->GetSize(), bStream::Endianess::Little, bStream::OpenMode::In);
			
			bStream::CFileStream dumpedMapData("dump.bin", bStream::Endianess::Little, bStream::OpenMode::Out);
			dumpedMapData.writeBytes(mapFile->GetData(), mapFile->GetSize());

			uint32_t objectsOffset = map.readUInt32() + 0x10;
			uint32_t modelOffset = map.readUInt32() + objectsOffset;
			uint32_t modelSize = map.readUInt32();

			bStream::CMemoryStream mapModel(mapFile->GetData() + modelOffset, modelSize, bStream::Endianess::Little, bStream::OpenMode::In);
			mMapModel.Load(mapModel);

			for(int i = 0; i < (modelOffset - objectsOffset) / 0x30; i++){
				std::cout << "Loading Object " << i << std::endl;
				map.seek(objectsOffset + (i * 0x30));
				uint32_t modelID = map.readUInt32();
				float x = (float)map.readUInt32() / (1 << 12);
				float y = (float)map.readUInt32() / (1 << 12);
				float z = (float)map.readUInt32() / (1 << 12);
				
				glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(x,y,z));

				mObjects.push_back({modelID, transform});

				if(!mModels.contains(modelID)){
					std::shared_ptr<Palkia::Nitro::File> model = buildModelArc.GetFileByIndex(modelID); //76 //518
				
					bStream::CMemoryStream modelFile(model->GetData(), model->GetSize(), bStream::Endianess::Little, bStream::OpenMode::In);
					Palkia::NSBMD objModel;
					objModel.Load(modelFile);
					mModels.insert({modelID, objModel});
				}
			}
		}
	ImGui::End();

	ImGui::SetNextWindowClass(&mainWindowOverride);


	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0,0));
	ImGui::SetNextWindowClass(&mainWindowOverride);
	ImGui::Begin("viewportWindow");
		ImVec2 winSize = ImGui::GetContentRegionAvail();
		ImVec2 cursorPos = ImGui::GetCursorScreenPos();
		
		glBindFramebuffer(GL_FRAMEBUFFER, mFbo);

		if(winSize.x != mPrevWinWidth || winSize.y != mPrevWinHeight){
			glDeleteTextures(1, &mViewTex);
			glDeleteTextures(1, &mPickTex);
			glDeleteRenderbuffers(1, &mRbo);

			glGenRenderbuffers(1, &mRbo);
			glBindRenderbuffer(GL_RENDERBUFFER, mRbo);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, (uint32_t)winSize.x, (uint32_t)winSize.y);

			glGenTextures(1, &mViewTex);
			glGenTextures(1, &mPickTex);

			glBindTexture(GL_TEXTURE_2D, mViewTex);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, (uint32_t)winSize.x, (uint32_t)winSize.y, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

			glBindTexture(GL_TEXTURE_2D, mPickTex);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_R32I, (uint32_t)winSize.x, (uint32_t)winSize.y, 0, GL_RED_INTEGER, GL_INT, nullptr);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mViewTex, 0);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, mPickTex, 0);
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, mRbo);

			assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

			GLenum attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_DEPTH_STENCIL_ATTACHMENT };
			glDrawBuffers(3, attachments);
		}
		
		glViewport(0, 0, (uint32_t)winSize.x, (uint32_t)winSize.y);
		glClearColor(0.100f, 0.261f, 0.402f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glEnable(GL_DEPTH_TEST);
		int32_t unused = 0;
		glClearTexImage(mPickTex, 0, GL_RED_INTEGER, GL_INT, &unused);

		mPrevWinWidth = winSize.x;
		mPrevWinHeight = winSize.y;
		
		glm::mat4 projection, view;
		projection = mCamera.GetProjectionMatrix();
		view = mCamera.GetViewMatrix();

		glm::mat4 modelView = projection * view;

		mMapModel.Render(modelView);

		for(auto obj : mObjects){
			mModels[obj.first].Render(modelView * obj.second);
		}

		cursorPos = ImGui::GetCursorScreenPos();
		ImGui::Image(reinterpret_cast<void*>(static_cast<uintptr_t>(mViewTex)), { winSize.x, winSize.y }, {0.0f, 1.0f}, {1.0f, 0.0f});

		if(ImGui::IsWindowFocused()){
			mViewportIsFocused = true;
		} else {
			mViewportIsFocused = false;
		}

		if(ImGui::IsItemClicked(0) && !ImGuizmo::IsOver()){
			ImVec2 mousePos = ImGui::GetMousePos();
				
			ImVec2 pickPos = {
				mousePos.x - cursorPos.x,
				winSize.y - (mousePos.y - cursorPos.y)
			};

			glPixelStorei(GL_PACK_ALIGNMENT, 1);
			glReadBuffer(GL_COLOR_ATTACHMENT1);
			uint32_t id = 0xFFFFFFFF;
			glReadPixels(static_cast<GLint>(pickPos.x), static_cast<GLint>(pickPos.y), 1, 1, GL_RED_INTEGER, GL_INT, (void*)&id);
		}

		ImGuizmo::BeginFrame();
		ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());
		ImGuizmo::SetRect(cursorPos.x, cursorPos.y, winSize.x, winSize.y);

        /*
		ImGuizmo::Manipulate(&mCamera.GetViewMatrix()[0][0], &mCamera.GetProjectionMatrix()[0][0], (ImGuizmo::OPERATION)mGizmoOperation, ImGuizmo::WORLD, &transform[0][0], &delta[0][0])){
		*/
		
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

	ImGui::End();
	ImGui::PopStyleVar();

}

void UPalkiaContext::RenderMainWindow(float deltaTime) {}

void UPalkiaContext::RenderMenuBar() {
	ImGui::BeginMainMenuBar();

	if (ImGui::BeginMenu("File")) {
		if (ImGui::MenuItem(ICON_FK_FOLDER_OPEN " Open...")) { }
		if (ImGui::MenuItem(ICON_FK_FLOPPY_O " Save...")) { }

		ImGui::Separator();
		ImGui::MenuItem(ICON_FK_WINDOW_CLOSE " Close");

		ImGui::EndMenu();
	}
    
	if (ImGui::BeginMenu("Edit")) {
		if(ImGui::MenuItem(ICON_FK_COG " Settings")){
			mOptionsOpen = true;
		}
		ImGui::EndMenu();
	}

	ImGui::EndMainMenuBar();

}
