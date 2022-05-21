#pragma once

#include <cstdint>

template<class Tevent>
class EventBase
{
public:
	EventBase();
	~EventBase();

	template<class... TArgs>
	void RaiseEvent(TArgs&&... Args);

	EventBase<Tevent>& operator+=(const Tevent& Rhs);
	EventBase<Tevent>& operator-=(const Tevent& Rhs);

private:
	// List of event pointers
	Tevent* EventStack;
	// Count of event pointers
	uint8_t StackSize;
};

template<class Tevent>
inline EventBase<Tevent>::EventBase()
	: EventStack(nullptr), StackSize(0)
{
}

template<class Tevent>
inline EventBase<Tevent>::~EventBase()
{
	if (EventStack)
		delete[] EventStack;
	EventStack = nullptr;
	StackSize = 0;
}

template<class Tevent>
inline EventBase<Tevent>& EventBase<Tevent>::operator+=(const Tevent& Rhs)
{
	auto NewBuffer = new Tevent[StackSize + 1];
	
	if (EventStack != nullptr)
	{
		std::memcpy(NewBuffer, EventStack, sizeof(void*) * StackSize);
		delete[] EventStack;
	}

	NewBuffer[StackSize] = Rhs;

	EventStack = NewBuffer;
	StackSize++;

	return *this;
}

template<class Tevent>
inline EventBase<Tevent>& EventBase<Tevent>::operator-=(const Tevent& Rhs)
{
	if (EventStack == nullptr)
		return *this;

	int32_t Index = -1;
	for (uint32_t i = 0; i < StackSize; i++)
	{
		if (EventStack[i] == Rhs)
		{
			Index = i;
			break;
		}
	}

	if (Index < 0)
		return *this;
	else if (StackSize == 1)
	{
		delete[] EventStack;
		EventStack = nullptr;
		StackSize--;
		return *this;
	}

	auto NewBuffer = new Tevent[StackSize - 1];

	std::memcpy(NewBuffer, EventStack, sizeof(void*) * Index);
	std::memcpy(NewBuffer + Index, EventStack + Index + 1, sizeof(void*) * (StackSize - Index - 1));

	delete[] EventStack;

	EventStack = NewBuffer;
	StackSize--;

	return *this;
}

// Define normal types here
using BasicEvent = EventBase<void(void)>;

template<class Tevent>
template<class... TArgs>
inline void EventBase<Tevent>::RaiseEvent(TArgs&&... Args)
{
	if (EventStack == nullptr)
		return;

	for (uint32_t i = 0; i < StackSize; i++)
		EventStack[i](std::forward<TArgs>(Args)...);
}
