#pragma once

#include "Input/InputCodes.h"
#include "EventStructures/InputEvent.h"
#include "EventStructures/WindowClosedEvent.h"
#include "EventStructures/WindowDestroyedEvent.h"
#include "EventStructures/WindowEndMoveEvent.h"
#include "EventStructures/WindowEnterFullscreenEvent.h"
#include "EventStructures/WindowExitFullscreenEvent.h"
#include "EventStructures/WindowLostFocusEvent.h"
#include "EventStructures/WindowMaximizedEvent.h"
#include "EventStructures/WindowMinimizedEvent.h"
#include "EventStructures/WindowMoveEvent.h"
#include "EventStructures/WindowReceivedFocusEvent.h"
#include "EventStructures/WindowResizedEvent.h"
#include "EventStructures/WindowRestoredEvent.h"

namespace EventSystem
{
	template <typename T>
	void SendEventImmediate(T&& event);

	template <typename T>
	void SubscribeToEvent(std::function<void(T&&)>&& subscription);
};