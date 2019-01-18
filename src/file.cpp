#include <fcrisan/native/file.hpp>
#include <fcrisan/native/error.hpp>
#include <Windows.h>
#include <safeint.h>

template <typename T> using safe_int = msl::utilities::SafeInt<T>;

namespace fcrisan::native {

	static_assert(sizeof(std::streamsize) == sizeof(LARGE_INTEGER));

	struct file::impl {
		bool ownsHandle;
		file_handle h;
	};

	file::~file() {
		try_close();
	}

	file::file(file_handle h, bool ownsHandle)
		: pimpl(new impl{ ownsHandle, h })
	{
		if (h == INVALID_HANDLE_VALUE)
			throw std::invalid_argument("bad file handle");
	}

	bool file::try_close() {
		if (!pimpl->ownsHandle)
			return true;
		if (pimpl->h == nullptr)
			return true;

		auto h = pimpl->h;
		pimpl->h = nullptr;
		pimpl->ownsHandle = false;

		clear_error();
		return ::CloseHandle(h);
	}

	void file::close() {
		if (!try_close())
			throw_error("cannot close file handle");
	}

	void file::rewind()  {
		LARGE_INTEGER zeroPos = { 0 };
		clear_error();
		if (!::SetFilePointerEx(pimpl->h, zeroPos, nullptr, FILE_BEGIN))
			throw_error("cannot rewind file");
	}

	std::streamsize file::position() const {
		static_assert(sizeof(std::streamsize) == sizeof(LARGE_INTEGER));
		LARGE_INTEGER zeroPos = { 0 };
		LARGE_INTEGER pos;
		clear_error();
		if (!::SetFilePointerEx(pimpl->h, zeroPos, &pos, FILE_CURRENT))
			throw_error("cannot get file position");
		return pos.QuadPart;
	}

	void file::position(std::streamsize pos) {
		static_assert(sizeof(std::streamsize) == sizeof(LARGE_INTEGER));
		LARGE_INTEGER newPos;
		newPos.QuadPart = pos;
		LARGE_INTEGER actualPos;
		clear_error();
		if (!::SetFilePointerEx(pimpl->h, newPos, &actualPos, FILE_BEGIN))
			throw_error("cannot set file position");
		if (newPos.QuadPart != actualPos.QuadPart)
			throw_error("cannot set file position");
	}

	std::streamsize file::size() const {
		static_assert(sizeof(std::streamsize) == sizeof(LARGE_INTEGER));
		LARGE_INTEGER fileSize;
		clear_error();
		if (!::GetFileSizeEx(pimpl->h, &fileSize))
			throw_error("cannot set file position");
		return fileSize.QuadPart;
	}

	file::operator file_handle() {
		return pimpl->h;
	}

	file_handle file::handle() {
		return pimpl->h;
	}

	std::size_t file::read(void *buffer, std::size_t size) {
		safe_int<DWORD> bytesToRead{ size };
		DWORD bytesRead;
		if (!::ReadFile(pimpl->h, buffer, bytesToRead, &bytesRead, nullptr)) {
			auto err = last_error();
			if (err != ERROR_BROKEN_PIPE)
				throw_error("cannot read from file", err);
		}
		return safe_int<size_t>(bytesRead);
	}

	void file::write(const void *buffer, std::size_t size) {
		safe_int<DWORD> bytesToWrite{ size };
		DWORD bytesWritten;
		if (!::WriteFile(pimpl->h, buffer, bytesToWrite, &bytesWritten, nullptr))
			throw_error("cannot write to file");
		if (bytesWritten != bytesToWrite)
			throw_error("partial write to file");
	}

	//---

	static_assert(sizeof(file_access) == sizeof(DWORD));
	static_assert(sizeof(share_mode) == sizeof(DWORD));
	static_assert(sizeof(create_mode) == sizeof(DWORD));

	std::shared_ptr<file> open_file(const nzstring *fileName, file_access access, share_mode share, create_mode creationMode) {
		clear_error();
		HANDLE h = ::CreateFileW(fileName, static_cast<DWORD>(access), static_cast<DWORD>(share), nullptr, static_cast<DWORD>(creationMode), 0, nullptr);
		if (h == INVALID_HANDLE_VALUE)
			throw_error("cannot open file");
		return std::make_shared<file>(h, true);
	}

	pipe create_anonymous_pipe() {
		HANDLE hWrite;
		HANDLE hRead;

		clear_error();
		if (!::CreatePipe(&hRead, &hWrite, nullptr, 0))
			throw_error("cannot create anonymous pipe");

		return pipe {
			std::make_shared<file>(hRead, true),
			std::make_shared<file>(hWrite, true)
		};
	}
}