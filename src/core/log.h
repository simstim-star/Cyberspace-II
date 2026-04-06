#pragma once

#include <algorithm>
#include <format>
#include <string>
#include <string_view>

namespace Sendai {
class Log {
  public:
	static constexpr size_t Capacity = 4096;

	inline void
	Append(std::wstring_view Text)
	{
		if (Text.empty() || _Buffer.size() >= Capacity) {
			return;
		}

		size_t SpaceLeft = Capacity - _Buffer.size();
		size_t ToAdd = min(Text.size(), SpaceLeft);

		_Buffer.append(Text.data(), ToAdd);
	}

	template <typename... Args>
	inline void
	Appendf(std::wformat_string<Args...> fmt, Args &&...args)
	{
		if (_Buffer.size() >= Capacity) {
			return;
		}
		std::wstring formatted = std::format(fmt, std::forward<Args>(args)...);
		Append(formatted);
	}

	inline size_t
	Length() const
	{
		return _Buffer.size();
	}

	inline const wchar_t *
	CStr() const
	{
		return _Buffer.c_str();
	}

	inline void
	Clear()
	{
		_Buffer.clear();
	}

  private:
	std::wstring _Buffer;
};

extern Log LOG;
} // namespace Sendai
