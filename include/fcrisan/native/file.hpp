#ifndef FCRISAN_NATIVE_FILE_HPP
#define FCRISAN_NATIVE_FILE_HPP

#pragma once

#include <iosfwd>
#include <memory>

namespace fcrisan::native {

#ifdef _WIN32
	/**
		Native operating system file handle type.
	*/
	typedef /*HANDLE*/ void* file_handle;
#else
#   error Not implemented for this platform.
#endif

	/**
		Thin wrapper over native operating system file functionality.
		
		Can be converted to a file handle so that you can use additional native operating system functionality.
	*/
	class file {
	public:
		file(file_handle, bool ownsHandle);

		virtual ~file();

		virtual bool try_close();
		virtual void close();

		virtual void rewind();
		virtual std::streamsize position() const;
		virtual void position(std::streamsize pos);
		virtual std::streamsize size() const;

		virtual std::size_t read(void *buffer, std::size_t size);
		virtual void write(const void *buffer, std::size_t size);

		virtual operator file_handle();
		virtual file_handle handle();

	protected:
		file() = default;

	private:
		file(const file &) = delete;
		file(const file &&) = delete;
		file & operator=(const file &) = delete;
		file & operator=(const file &&) = delete;

		struct impl;
		std::unique_ptr<impl> pimpl;
	};


#ifdef _WIN32

	/**
		File access enumeration, mapping directly over operating system functionality.
	*/
	enum class file_access : unsigned long /*DWORD*/ {
		read = 0x80000000 /*GENERIC_READ*/,
		write = 0x40000000 /*GENERIC_WRITE*/,
		rw = read | write
	};

	enum class share_mode : unsigned long /*DWORD*/ {
		none = 0,
		read = 1 /*FILE_SHARE_READ*/,
		write = 2 /*FILE_SHARE_WRITE*/,
		del = 4 /*FILE_SHARE_DELETE*/,
	};

	enum class create_mode : unsigned long /*DWORD*/ {
		create_always = 2 /*CREATE_ALWAYS*/,
		create_new = 1 /*CREATE_NEW*/,
		open_always = 4 /*OPEN_ALWAYS*/,
		open_existing = 3 /*OPEN_EXISTING*/,
		truncate_existing = 5 /*TRUNCATE_EXISTING*/,
	};

#else
#   error Not implemented for this platform.
#endif

	/**
		Opens a file using native operating system functionality.
	*/
	std::shared_ptr<file> open_file(const char *fileName, file_access, share_mode, create_mode);
	std::shared_ptr<file> open_file(const wchar_t *fileName, file_access, share_mode, create_mode);

	/**
		Holds the two ends of a pipe.
	*/
	struct pipe {
		std::shared_ptr<file> read;
		std::shared_ptr<file> write;
	};

	/**
		Creates an anonymous pipe.
	*/
	pipe create_anonymous_pipe();

} // namespace fcrisan::native

#endif // FCRISAN_NATIVE_FILE_HPP
