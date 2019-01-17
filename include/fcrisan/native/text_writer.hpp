#ifndef FCRISAN_NATIVE_TEXT_WRITER_HPP
#define FCRISAN_NATIVE_TEXT_WRITER_HPP

#pragma once

#include <memory>
#include <fcrisan/native/code_page.hpp>

namespace fcrisan::native {

	class file;

	class text_writer {
	public:
		text_writer(file &outFile, code_page_id cp);
		text_writer(text_writer &other) = delete;
		text_writer(text_writer &&other) = delete;
		text_writer & operator=(text_writer &other) = delete;
		text_writer & operator=(text_writer &&other) = delete;

		virtual ~text_writer();

		void write(const char *what, std::size_t count, code_page_id text_cp = code_pages::utf8);
		void write(const wchar_t *what, std::size_t count);

	private:
		struct impl;
		std::unique_ptr<impl> pimpl;
	};

}

#endif // FCRISAN_NATIVE_TEXT_WRITER_HPP
