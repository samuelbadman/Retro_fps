#include "Pch.h"
#include "Console.h"
#include "Window/Window.h"
#include "EventSystem/EventSystem.h"
#include "Input/Gamepad/GamepadManager.h"
#include "Renderer/Renderer.h"
#include "Audio/Audio.h"
#include "Maths/Maths.h"

#include "Game/World.h"
#include "Game/GameInput.h"
#include "Game/GameState.h"
#include "Game/Physics.h"
#include "Game/Billboard.h"
#include "Game/LevelGoal.h"
#include "Game/EnemyAI.h"
#include "Game/Levitate.h"

#include "Game/Components/TransformComponent.h"
#include "Game/Components/CameraComponent.h"
#include "Game/Components/RigidBodyComponent.h"

#include "Game/Levels/MainMenuLevel.h"

static bool gQuit{ false };
static bool gMainWindowMinimized{ false };

int WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
#ifdef _DEBUG
	Console::CreateConsole(4096);
#endif // _DEBUG

	// Create the game window
	auto mainWindow = std::make_unique<Window>();
	// Check the game window was initialised with a resolution, no parent, a title and a window style succesfully
	if (!mainWindow->Init({ 1920, 1080 }, nullptr, L"WindowClass1", L"Game", EWindowStyle::NO_RESIZE_WINDOWED))
	{
		return 1;
	}
	mainWindow->Show();

	// If in release configuration
#ifdef NDEBUG
	// Make the window fullscreen
	mainWindow->SetFullscreen(true);

	// Capture the cursor in the window
	if (!mainWindow->CaptureCursor())
	{
		return 1;
	}

	// Hide the cursor
	ShowCursor(false);
#endif

	// Subscribe handler for window destroyed events
	EventSystem::SubscribeToEvent<WindowDestroyedEvent>([pMainWindow{ mainWindow.get() }](WindowDestroyedEvent&& event) {
		// If the destroyed window is the main window
		if (event.pWindow == pMainWindow)
		{
			// Quit the application
			gQuit = true;
		}
	});

	// Subscribe handler for window minimzed events
	EventSystem::SubscribeToEvent<WindowMinimizedEvent>([pMainWindow{ mainWindow.get() }](WindowMinimizedEvent&& event) {
		// Check if the minimized window is the main window
		if (event.pWindow == pMainWindow)
		{
			// Set the main window is minimized flag
			gMainWindowMinimized = true;
		}
	});

	// Subscribe handler for window restored events
	EventSystem::SubscribeToEvent<WindowRestoredEvent>([pMainWindow{ mainWindow.get() }](WindowRestoredEvent&& event) {
		// Check if the restored window is the main window
		if (event.pWindow == pMainWindow)
		{
			// Clear the main window is minimized flag
			gMainWindowMinimized = false;
		}
	});

	// Subscribe handler for input events for the application
	EventSystem::SubscribeToEvent<InputEvent>([pMainWindow{ mainWindow.get() }](InputEvent&& event)
		{
			// Close the main window if escape key is pressed
			if (event.Input == InputCodes::Escape && event.Data == 1.0f)
			{
				pMainWindow->Close();
			}
		});

	// Subscribe handler for input events for the application
	EventSystem::SubscribeToEvent<InputEvent>(GameInput::HandleInputEvent);

	// Initialise the renderer
	RECT windowClientAreaRect;
	mainWindow->GetClientAreaRect(windowClientAreaRect);
	if (!Renderer::Init(
		{windowClientAreaRect.right - windowClientAreaRect.left, windowClientAreaRect.bottom - windowClientAreaRect.top}, 
		mainWindow->GetHandle()))
	{
		return 1;
	}
	Renderer::SetVulkanDebugReportLevel(Renderer::EVulkanDebugReportLevel::ERR);

	// Initialise audio
	if (!Audio::Init())
	{
		return 1;
	}

	// Load missing texture fallback texture
	uint32_t fallbackTextureID;
	if (!Renderer::LoadTexture("Assets/Engine/checker.png", true, &fallbackTextureID))
	{
		return 1;
	}

	// Init persistent game state
	if (!GameState::Init())
	{
		return 1;
	}

	// Load main menu level
	if (!World::LoadLevel(std::move(std::make_unique<MainMenuLevel>()), true))
	{
		return 1;
	}

	// Begin game state
	GameState::Begin();

	// Enter main loop
	using namespace std::chrono_literals;
	using clock = std::chrono::high_resolution_clock;
	constexpr std::chrono::nanoseconds fixedTimestep( 16ms );
	std::chrono::nanoseconds lag(0ns);
	auto currentTime = clock::now();
	while (!gQuit)
	{
		// Calculate delta time in milliseconds
		std::chrono::duration<float, std::milli> deltaTime = clock::now() - currentTime;
		currentTime = clock::now();
		lag += std::chrono::duration_cast<std::chrono::nanoseconds>(deltaTime);

		auto fDeltaTime = deltaTime.count();
		World::SetWorldDeltaTime(fDeltaTime);

		// Get the loaded level and the level's HUD
		auto& loadedLevel = World::GetLoadedLevel();
		auto* pHUD = loadedLevel.GetHUDClassInstance();

		// Dispatch operating system messages
		MSG msg{};
		while (::PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}

		// Early exit if the main window was closed
		if (gQuit)
		{
			break;
		}

		// Generate gamepad events for frame
		GamepadManager::RefreshGamepadPorts();

		// Poll game inputs
		GameInput::PollInputs();

		// Tick the game state
		GameState::Tick(fDeltaTime);

		// Tick the loaded level
		loadedLevel.Tick(fDeltaTime);

		while (lag >= fixedTimestep)
		{
			lag -= fixedTimestep;

			// Fixed tick the loaded level
			loadedLevel.TickFixed();
		}

		// Update game systems
		Levitate::Update(loadedLevel);
		if (GameState::GetCurrentState() == GameState::EState::IN_GAME)
		{
			Billboard::Update(loadedLevel);
			Physics::Update(loadedLevel);
			LevelGoal::Update(loadedLevel);
			EnemyAI::Update(loadedLevel);
		}

		// Update audio
		Maths::Transform possessedEntityTransform{};
		glm::vec3 possessedEntityVelocity{};

		const auto* possessedEntity{ loadedLevel.GetPossessedEntity() };
		auto possessedEntityValid{ possessedEntity->Valid() };

		if (possessedEntityValid)
		{
			possessedEntityTransform = possessedEntity->GetComponent<TransformComponent>().Transform;
			possessedEntityVelocity = possessedEntity->GetComponent<RigidBodyComponent>().Velocity;
		}

		Audio::SetListener(possessedEntityTransform, possessedEntityVelocity);

		Audio::Update3DEmitters(loadedLevel);

		// Check the main window is not minimized
		if (!gMainWindowMinimized)
		{
			// Begin a frame
			if (!Renderer::BeginFrame(loadedLevel.GetDirectionalLight()))
			{
				assert(false && "Renderer failed to begin a frame.");
			}

			// Get the possessed transform and camera component
			TransformComponent possessedTransformComponent{};
			CameraComponent possessedCameraComponent{};

			if (possessedEntityValid)
			{
				possessedTransformComponent = loadedLevel.GetPossessedEntity()->GetComponent<TransformComponent>();
				possessedCameraComponent = loadedLevel.GetPossessedEntity()->GetComponent<CameraComponent>();
			}

			// Begin render pass with the main scene camera
			Renderer::BeginRenderPass(possessedTransformComponent.Transform.Position,
				possessedTransformComponent.Transform.Rotation,
				possessedCameraComponent.CameraSettings);

			// Submit the level
			if (!Renderer::SubmitLevel(loadedLevel))
			{
				assert(false && "Renderer failed to render submitted level.");
			}

			// Check if the loaded level's HUD instance is valid
			if (pHUD != nullptr)
			{
				// Begin render pass for HUD
				Renderer::BeginRenderPass(pHUD->GetHUDCameraPosition(),
					pHUD->GetHUDCameraRotation(),
					pHUD->GetHUDCameraSettings());

				// Submit level's HUD
				Renderer::SubmitHUD(*pHUD);
			}

			// End frame
			if (!Renderer::EndFrame())
			{
				assert(false && "Renderer failed to end a frame.");
			}

			// Check if a level is scheduled to be loaded
			if (World::IslevelScheduled())
			{
				// Load the scheduled level
				if (!World::LoadScheduledLevel())
				{
					assert(false && "Failed to load the scheduled level.");
				}
			}
		}
	}

	// End and unload the loaded level
	auto& loadedLevel = World::GetLoadedLevel();
	loadedLevel.UnLoad();

	// Shutdown persistent game state
	GameState::Shutdown();

	// Shutdown the renderer
	if (!Renderer::Shutdown())
	{
		return 1;
	}

	// Shutdown audio
	Audio::Shutdown();

#ifdef _DEBUG
	Console::ReleaseConsole();
#endif // _DEBUG

	return 0;
}