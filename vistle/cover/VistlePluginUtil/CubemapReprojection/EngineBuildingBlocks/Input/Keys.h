// EngineBuildingBlocks/Input/Keys.h

#ifndef _ENGINEBUILDINGBLOCKS_KEYS_H_INCLUDED_
#define _ENGINEBUILDINGBLOCKS_KEYS_H_INCLUDED_

#include <Core/DataStructures/SimpleTypeVector.hpp>

#include <map>

namespace EngineBuildingBlocks
{
	namespace Input
	{
		enum class Keys : short
		{
			// The unknown key: no (specific -> general) mapping is found for the key.
			Unknown = -1,

			// Printable keys.
			Space, Apostrophe, Comma, Minus, Period, Slash, Semicolon, Equal, LeftBracket, BackSlash, RightBracket, GraveAccent,
			World1, World2,
			Key0, Key1, Key2, Key3, Key4, Key5, Key6, Key7, Key8, Key9,
			A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
				
			// Function keys.
			Escape, Enter, Tab, Backspace, Insert, Delete, Right, Left, Down, Up, PageUp, PageDown, Home, End,
			CapsLock, ScrollLock, NumLock, PrintScreen, Pause,
			F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
			KeyPad0, KeyPad1, KeyPad2, KeyPad3, KeyPad4, KeyPad5, KeyPad6, KeyPad7, KeyPad8, KeyPad9,
			KeyPadDecimal, KeyPadDivide, KeyPadMultiply, KeyPadSubtract, KeyPadAdd, KeyPadEnter, KeyPadEqual,
			LeftShift, LeftControl, LeftAlt, LeftSuper, RightShift, RightControl, RightAlt, RightSuper, Menu,

			COUNT
		};

		class KeyMap
		{
		protected:

			std::map<int, Keys> m_SpecificToGeneral;
			Core::SimpleTypeVectorU<int> m_GeneralToSpecific;

		public:

			Keys GetGeneralKey(int specificKey) const;
			int GetSpecificKey(Keys generalKey) const;
		};
	}
}

#endif