/*
Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 3.0 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "chat.h"

#include <algorithm>
#include <cctype>
#include <sstream>

#include "config.h"
#include "debug.h"
#include "util/strfnd.h"
#include "util/string.h"
#include "util/numeric.h"

ChatBuffer::ChatBuffer(u32 scrollback):
	m_scrollback(scrollback)
{
	if (m_scrollback == 0)
		m_scrollback = 1;
	m_empty_formatted_line.first = true;
	m_empty_formatted_line.line_index = 0;
}

void ChatBuffer::addLine(const std::wstring &name, const std::wstring &text)
{
	m_lines_modified = true;

	ChatLine line(name, text, m_current_line_index);
	m_unformatted.push_back(line);
	m_current_line_index++;

	if (m_rows > 0) {
		// m_formatted is valid and must be kept valid
		bool scrolled_at_bottom = (m_scroll == getBottomScrollPos());
		u32 num_added = formatChatLine(line, m_cols, m_formatted);
		if (scrolled_at_bottom)
			m_scroll += num_added;
	}

	// Limit number of lines by m_scrollback
	if (m_unformatted.size() > m_scrollback) {
		deleteOldest(m_unformatted.size() - m_scrollback);
	}
}

void ChatBuffer::clear()
{
	m_unformatted.clear();
	m_formatted.clear();
	m_current_line_index = 0;
	m_scroll = 0;
	m_lines_modified = true;
}

u32 ChatBuffer::getLineCount() const
{
	return m_unformatted.size();
}

const ChatLine& ChatBuffer::getLine(u32 index) const
{
	assert(index < getLineCount());	// pre-condition
	return m_unformatted[index];
}

void ChatBuffer::step(f32 dtime)
{
	for (ChatLine &line : m_unformatted) {
		line.age += dtime;
	}
}

void ChatBuffer::deleteOldest(u32 count)
{
	bool at_bottom = (m_scroll == getBottomScrollPos());

	u32 del_unformatted = 0;
	u32 del_formatted = 0;

	while (count > 0 && del_unformatted < m_unformatted.size()) {
		++del_unformatted;

		// keep m_formatted in sync
		if (del_formatted < m_formatted.size()) {
			sanity_check(m_formatted[del_formatted].first);
			++del_formatted;
			while (del_formatted < m_formatted.size() &&
					!m_formatted[del_formatted].first)
				++del_formatted;
		}

		--count;
	}

	m_unformatted.erase(m_unformatted.begin(), m_unformatted.begin() + del_unformatted);
	m_formatted.erase(m_formatted.begin(), m_formatted.begin() + del_formatted);

	if (del_unformatted > 0)
		m_lines_modified = true;

	if (at_bottom)
		m_scroll = getBottomScrollPos();
	else
		scrollAbsolute(m_scroll - del_formatted);

	m_del_formatted += del_formatted;
}

void ChatBuffer::deleteByAge(f32 maxAge)
{
	u32 count = 0;
	while (count < m_unformatted.size() && m_unformatted[count].age > maxAge)
		++count;
	deleteOldest(count);
}

u32 ChatBuffer::getRows() const
{
	return m_rows;
}

void ChatBuffer::scrollTop()
{
	m_scroll = getTopScrollPos();
}

void ChatBuffer::reformat(u32 cols, u32 rows)
{
	if (cols == 0 || rows == 0)
	{
		// Clear formatted buffer
		m_cols = 0;
		m_rows = 0;
		m_scroll = 0;
		m_formatted.clear();
	}
	else if (cols != m_cols || rows != m_rows)
	{
		// TODO: Avoid reformatting ALL lines (even invisible ones)
		// each time the console size changes.

		// Find out the scroll position in *unformatted* lines
		u32 restore_scroll_unformatted = 0;
		u32 restore_scroll_formatted = 0;
		bool at_bottom = (m_scroll == getBottomScrollPos());
		if (!at_bottom)
		{
			for (s32 i = 0; i < m_scroll; ++i)
			{
				if (m_formatted[i].first)
					++restore_scroll_unformatted;
			}
		}

		// If number of columns change, reformat everything
		if (cols != m_cols)
		{
			m_formatted.clear();
			for (u32 i = 0; i < m_unformatted.size(); ++i)
			{
				if (i == restore_scroll_unformatted)
					restore_scroll_formatted = m_formatted.size();
				formatChatLine(m_unformatted[i], cols, m_formatted);
			}
		}

		// Update the console size
		m_cols = cols;
		m_rows = rows;

		// Restore the scroll position
		if (at_bottom)
		{
			scrollBottom();
		}
		else
		{
			scrollAbsolute(restore_scroll_formatted);
		}
	}
}

const ChatFormattedLine& ChatBuffer::getFormattedLine(u32 row) const
{
	s32 index = m_scroll + (s32) row;
	if (index >= 0 && index < (s32) m_formatted.size())
		return m_formatted[index];

	return m_empty_formatted_line;
}

void ChatBuffer::scroll(s32 rows)
{
	scrollAbsolute(m_scroll + rows);
}

void ChatBuffer::scrollAbsolute(s32 scroll)
{
	s32 top = getTopScrollPos();
	s32 bottom = getBottomScrollPos();

	m_scroll = scroll;
	if (m_scroll < top)
		m_scroll = top;
	if (m_scroll > bottom)
		m_scroll = bottom;
}

void ChatBuffer::scrollBottom()
{
	m_scroll = getBottomScrollPos();
}

u32 ChatBuffer::formatChatLine(const ChatLine& line, u32 cols,
		std::vector<ChatFormattedLine>& destination) const
{
	u32 num_added = 0;
	std::vector<ChatFormattedFragment> next_frags;
	ChatFormattedLine next_line;
	ChatFormattedFragment temp_frag;
	u32 out_column = 0;
	u32 in_pos = 0;
	u32 hanging_indentation = 0;

	// Format the sender name and produce fragments
	if (!line.name.empty()) {
		temp_frag.text = L"<";
		temp_frag.column = 0;
		//temp_frag.bold = 0;
		next_frags.push_back(temp_frag);
		temp_frag.text = line.name;
		temp_frag.column = 0;
		//temp_frag.bold = 1;
		next_frags.push_back(temp_frag);
		temp_frag.text = L"> ";
		temp_frag.column = 0;
		//temp_frag.bold = 0;
		next_frags.push_back(temp_frag);
	}

	std::wstring name_sanitized = line.name.c_str();

	// Choose an indentation level
	if (line.name.empty()) {
		// Server messages
		hanging_indentation = 0;
	} else if (name_sanitized.size() + 3 <= cols/2) {
		// Names shorter than about half the console width
		hanging_indentation = line.name.size() + 3;
	} else {
		// Very long names
		hanging_indentation = 2;
	}
	//EnrichedString line_text(line.text);

	next_line.first = true;
	next_line.line_index = line.line_index;
	bool text_processing = false;

	// Produce fragments and layout them into lines
	while (!next_frags.empty() || in_pos < line.text.size())
	{
		// Layout fragments into lines
		while (!next_frags.empty())
		{
			ChatFormattedFragment& frag = next_frags[0];
			if (frag.text.size() <= cols - out_column)
			{
				// Fragment fits into current line
				frag.column = out_column;
				next_line.fragments.push_back(frag);
				out_column += frag.text.size();
				next_frags.erase(next_frags.begin());
			}
			else
			{
				// Fragment does not fit into current line
				// So split it up
				temp_frag.text = frag.text.substr(0, cols - out_column);
				temp_frag.column = out_column;
				//temp_frag.bold = frag.bold;
				next_line.fragments.push_back(temp_frag);
				frag.text = frag.text.substr(cols - out_column);
				out_column = cols;
			}
			if (out_column == cols || text_processing)
			{
				// End the current line
				destination.push_back(next_line);
				num_added++;
				next_line.fragments.clear();
				next_line.first = false;

				out_column = text_processing ? hanging_indentation : 0;
			}
		}

		// Produce fragment
		if (in_pos < line.text.size())
		{
			u32 remaining_in_input = line.text.size() - in_pos;
			u32 remaining_in_output = cols - out_column;

			// Determine a fragment length <= the minimum of
			// remaining_in_{in,out}put. Try to end the fragment
			// on a word boundary.
			u32 frag_length = 1, space_pos = 0;
			while (frag_length < remaining_in_input &&
					frag_length < remaining_in_output)
			{
				if (iswspace(line.text.getString()[in_pos + frag_length]))
					space_pos = frag_length;
				++frag_length;
			}
			if (space_pos != 0 && frag_length < remaining_in_input)
				frag_length = space_pos + 1;

			temp_frag.text = line.text.substr(in_pos, frag_length);
			temp_frag.column = 0;
			//temp_frag.bold = 0;
			next_frags.push_back(temp_frag);
			in_pos += frag_length;
			text_processing = true;
		}
	}

	// End the last line
	if (num_added == 0 || !next_line.fragments.empty())
	{
		destination.push_back(next_line);
		num_added++;
	}

	return num_added;
}

s32 ChatBuffer::getTopScrollPos() const
{
	s32 formatted_count = (s32) m_formatted.size();
	s32 rows = (s32) m_rows;
	if (rows == 0)
		return 0;

	if (formatted_count <= rows)
		return formatted_count - rows;

	return 0;
}

s32 ChatBuffer::getBottomScrollPos() const
{
	s32 formatted_count = (s32) m_formatted.size();
	s32 rows = (s32) m_rows;
	if (rows == 0)
		return 0;

	return formatted_count - rows;
}

void ChatBuffer::resize(u32 scrollback)
{
	m_scrollback = scrollback;
	if (m_unformatted.size() > m_scrollback)
		deleteOldest(m_unformatted.size() - m_scrollback);
}


ChatPrompt::ChatPrompt(const std::wstring &prompt, u32 history_limit):
	m_prompt(prompt),
	m_history_limit(history_limit)
{
}

void ChatPrompt::input(wchar_t ch)
{
	m_line.insert(m_cursor, 1, ch);
	m_cursor++;
	clampView();
	m_nick_completion_start = 0;
	m_nick_completion_end = 0;
	m_line_modified = true;
}

void ChatPrompt::input(const std::wstring &str)
{
	m_line.insert(m_cursor, str);
	m_cursor += str.size();
	clampView();
	m_nick_completion_start = 0;
	m_nick_completion_end = 0;
	m_line_modified = true;
}

void ChatPrompt::addToHistory(const std::wstring &line)
{
	if (!line.empty() &&
			(m_history.size() == 0 || m_history.back() != line)) {
		// Remove all duplicates
		m_history.erase(std::remove(m_history.begin(), m_history.end(),
			line), m_history.end());
		// Push unique line
		m_history.push_back(line);
	}
	if (m_history.size() > m_history_limit)
		m_history.erase(m_history.begin());
	m_history_index = m_history.size();
}

void ChatPrompt::clear()
{
	m_line.clear();
	m_view = 0;
	m_cursor = 0;
	m_nick_completion_start = 0;
	m_nick_completion_end = 0;
	m_line_modified = true;
}

std::wstring ChatPrompt::replace(const std::wstring &line)
{
	std::wstring old_line = m_line;
	m_line =  line;
	m_view = m_cursor = line.size();
	clampView();
	m_nick_completion_start = 0;
	m_nick_completion_end = 0;
	m_line_modified = true;
	return old_line;
}

void ChatPrompt::historyPrev()
{
	if (m_history_index != 0)
	{
		--m_history_index;
		replace(m_history[m_history_index]);
	}
}

void ChatPrompt::historyNext()
{
	if (m_history_index + 1 >= m_history.size())
	{
		m_history_index = m_history.size();
		replace(L"");
	}
	else
	{
		++m_history_index;
		replace(m_history[m_history_index]);
	}
}

void ChatPrompt::nickCompletion(const std::list<std::string>& names, bool backwards)
{
	// Two cases:
	// (a) m_nick_completion_start == m_nick_completion_end == 0
	//     Then no previous nick completion is active.
	//     Get the word around the cursor and replace with any nick
	//     that has that word as a prefix.
	// (b) else, continue a previous nick completion.
	//     m_nick_completion_start..m_nick_completion_end are the
	//     interval where the originally used prefix was. Cycle
	//     through the list of completions of that prefix.
	u32 prefix_start = m_nick_completion_start;
	u32 prefix_end = m_nick_completion_end;
	bool initial = (prefix_end == 0);
	if (initial)
	{
		// no previous nick completion is active
		prefix_start = prefix_end = m_cursor;
		while (prefix_start > 0 && !iswspace(m_line[prefix_start-1]))
			--prefix_start;
		while (prefix_end < m_line.size() && !iswspace(m_line[prefix_end]))
			++prefix_end;
		if (prefix_start == prefix_end)
			return;
	}
	std::wstring prefix = m_line.substr(prefix_start, prefix_end - prefix_start);

	// find all names that start with the selected prefix
	std::vector<std::wstring> completions;
	for (const std::string &name : names) {
		std::wstring completion = utf8_to_wide(name);
		if (str_starts_with(completion, prefix, true)) {
			if (prefix_start == 0)
				completion += L": ";
			completions.push_back(completion);
		}
	}

	if (completions.empty())
		return;

	// find a replacement string and the word that will be replaced
	u32 word_end = prefix_end;
	u32 replacement_index = 0;
	if (!initial)
	{
		while (word_end < m_line.size() && !iswspace(m_line[word_end]))
			++word_end;
		std::wstring word = m_line.substr(prefix_start, word_end - prefix_start);

		// cycle through completions
		for (u32 i = 0; i < completions.size(); ++i)
		{
			if (str_equal(word, completions[i], true))
			{
				if (backwards)
					replacement_index = i + completions.size() - 1;
				else
					replacement_index = i + 1;
				replacement_index %= completions.size();
				break;
			}
		}
	}
	std::wstring replacement = completions[replacement_index];
	if (word_end < m_line.size() && iswspace(m_line[word_end]))
		++word_end;

	// replace existing word with replacement word,
	// place the cursor at the end and record the completion prefix
	m_line.replace(prefix_start, word_end - prefix_start, replacement);
	m_cursor = prefix_start + replacement.size();
	clampView();
	m_nick_completion_start = prefix_start;
	m_nick_completion_end = prefix_end;
	m_line_modified = true;
}

void ChatPrompt::reformat(u32 cols)
{
	if (cols <= m_prompt.size())
	{
		m_cols = 0;
		m_view = m_cursor;
	}
	else
	{
		s32 length = m_line.size();
		bool was_at_end = (m_view + m_cols >= length + 1);
		m_cols = cols - m_prompt.size();
		if (was_at_end)
			m_view = length;
		clampView();
	}
}

std::wstring ChatPrompt::getVisiblePortion() const
{
	return m_prompt + m_line.substr(m_view, m_cols);
}

s32 ChatPrompt::getVisibleCursorPosition() const
{
	return m_cursor - m_view + m_prompt.size();
}

void ChatPrompt::cursorOperation(CursorOp op, CursorOpDir dir, CursorOpScope scope)
{
	s32 old_cursor = m_cursor;
	s32 new_cursor = m_cursor;

	s32 length = m_line.size();
	s32 increment = (dir == CURSOROP_DIR_RIGHT) ? 1 : -1;

	switch (scope) {
	case CURSOROP_SCOPE_CHARACTER:
		new_cursor += increment;
		break;
	case CURSOROP_SCOPE_WORD:
		if (dir == CURSOROP_DIR_RIGHT) {
			// skip one word to the right
			while (new_cursor < length && iswspace(m_line[new_cursor]))
				new_cursor++;
			while (new_cursor < length && !iswspace(m_line[new_cursor]))
				new_cursor++;
			while (new_cursor < length && iswspace(m_line[new_cursor]))
				new_cursor++;
		} else {
			// skip one word to the left
			while (new_cursor >= 1 && iswspace(m_line[new_cursor - 1]))
				new_cursor--;
			while (new_cursor >= 1 && !iswspace(m_line[new_cursor - 1]))
				new_cursor--;
		}
		break;
	case CURSOROP_SCOPE_LINE:
		new_cursor += increment * length;
		break;
	case CURSOROP_SCOPE_SELECTION:
		break;
	}

	new_cursor = MYMAX(MYMIN(new_cursor, length), 0);

	switch (op) {
	case CURSOROP_MOVE:
		m_cursor = new_cursor;
		m_cursor_len = 0;
		break;
	case CURSOROP_DELETE:
		if (m_cursor_len > 0) { // Delete selected text first
			m_line.erase(m_cursor, m_cursor_len);
		} else {
			m_cursor = MYMIN(new_cursor, old_cursor);
			m_line.erase(m_cursor, abs(new_cursor - old_cursor));
		}
		m_cursor_len = 0;
		m_line_modified = true;
		break;
	case CURSOROP_SELECT:
		if (scope == CURSOROP_SCOPE_LINE) {
			m_cursor = 0;
			m_cursor_len = length;
		} else {
			m_cursor = MYMIN(new_cursor, old_cursor);
			m_cursor_len += abs(new_cursor - old_cursor);
			m_cursor_len = MYMIN(m_cursor_len, length - m_cursor);
		}
		break;
	}

	clampView();

	m_nick_completion_start = 0;
	m_nick_completion_end = 0;
}

void ChatPrompt::setCursorPos(int cursor_pos)
{
	s32 length = m_line.size();
	m_cursor = MYMAX(MYMIN(cursor_pos, length), 0);
	m_cursor_len = 0;

	clampView();

	m_nick_completion_start = 0;
	m_nick_completion_end = 0;
}

void ChatPrompt::setViewPosition(int view)
{
	m_view = view;

	clampView();
}

void ChatPrompt::clampView()
{
	s32 length = m_line.size();
	if (length + 1 <= m_cols)
	{
		m_view = 0;
	}
	else
	{
		m_view = MYMIN(m_view, length + 1 - m_cols);
		m_view = MYMIN(m_view, m_cursor);
		m_view = MYMAX(m_view, m_cursor - m_cols + 1);
		m_view = MYMAX(m_view, 0);
	}
}



ChatBackend::ChatBackend():
	m_console_buffer(500),
	m_recent_buffer(6),
	m_prompt(L"]", 500)
{
}

void ChatBackend::addMessage(const std::wstring &name, std::wstring text)
{
	// Note: A message may consist of multiple lines, for example the MOTD.
	text = translate_string(text);
	WStrfnd fnd(text);
	while (!fnd.at_end())
	{
		std::wstring line = fnd.next(L"\n");
		m_console_buffer.addLine(name, line);
		m_recent_buffer.addLine(name, line);
	}
}

void ChatBackend::addUnparsedMessage(std::wstring message)
{
	// TODO: Remove the need to parse chat messages client-side, by sending
	// separate name and text fields in TOCLIENT_CHAT_MESSAGE.

	if (message.size() >= 2 && message[0] == L'<')
	{
		std::size_t closing = message.find_first_of(L'>', 1);
		if (closing != std::wstring::npos &&
				closing + 2 <= message.size() &&
				message[closing+1] == L' ')
		{
			std::wstring name = message.substr(1, closing - 1);
			std::wstring text = message.substr(closing + 2);
			addMessage(name, text);
			return;
		}
	}

	// Unable to parse, probably a server message.
	addMessage(L"", message);
}

ChatBuffer& ChatBackend::getConsoleBuffer()
{
	return m_console_buffer;
}

ChatBuffer& ChatBackend::getRecentBuffer()
{
	return m_recent_buffer;
}

EnrichedString ChatBackend::getRecentChat() const
{
	EnrichedString result;
	for (u32 i = 0; i < m_recent_buffer.getLineCount(); ++i) {
		const ChatLine& line = m_recent_buffer.getLine(i);
		if (i != 0)
			result += L"\n";
		if (!line.name.empty()) {
			result += L"<";
			result += line.name;
			result += L"> ";
		}
		result += line.text;
	}
	return result;
}

ChatPrompt& ChatBackend::getPrompt()
{
	return m_prompt;
}

void ChatBackend::reformat(u32 cols, u32 rows)
{
	m_console_buffer.reformat(cols, rows);

	// no need to reformat m_recent_buffer, its formatted lines
	// are not used

	m_prompt.reformat(cols);
}

void ChatBackend::clearRecentChat()
{
	m_recent_buffer.clear();
}


void ChatBackend::applySettings()
{
	u32 recent_lines = g_settings->getU32("recent_chat_messages");
	recent_lines = rangelim(recent_lines, 2, 20);
	m_recent_buffer.resize(recent_lines);
}

void ChatBackend::step(float dtime)
{
	m_recent_buffer.step(dtime);
	m_recent_buffer.deleteByAge(60.0);

	// no need to age messages in anything but m_recent_buffer
}

void ChatBackend::scroll(s32 rows)
{
	m_console_buffer.scroll(rows);
}

void ChatBackend::scrollPageDown()
{
	m_console_buffer.scroll(m_console_buffer.getRows());
}

void ChatBackend::scrollPageUp()
{
	m_console_buffer.scroll(-(s32)m_console_buffer.getRows());
}
