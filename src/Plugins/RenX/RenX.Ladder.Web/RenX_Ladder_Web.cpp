/**
 * Copyright (C) 2016 Jessica James.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Written by Jessica James <jessica.aj@outlook.com>
 */

#include "Jupiter/IRC_Client.h"
#include "Jupiter/HTTP.h"
#include "Jupiter/HTTP_QueryString.h"
#include "HTTPServer.h"
#include "RenX_Tags.h"
#include "RenX_Ladder_Web.h"

using namespace Jupiter::literals;

bool RenX_Ladder_WebPlugin::initialize()
{
	RenX_Ladder_WebPlugin::ladder_page_name = this->config.get("LadderPageName"_jrs, ""_jrs);
	RenX_Ladder_WebPlugin::search_page_name = this->config.get("SearchPageName"_jrs, "search"_jrs);
	RenX_Ladder_WebPlugin::profile_page_name = this->config.get("ProfilePageName"_jrs, "profile"_jrs);
	RenX_Ladder_WebPlugin::web_hostname = this->config.get("Hostname"_jrs, ""_jrs);
	RenX_Ladder_WebPlugin::web_path = this->config.get("Path"_jrs, "/"_jrs);

	this->init();

	/** Initialize content */
	Jupiter::HTTP::Server &server = getHTTPServer();

	Jupiter::HTTP::Server::Content *content = new Jupiter::HTTP::Server::Content(RenX_Ladder_WebPlugin::ladder_page_name, handle_ladder_page);
	content->language = &Jupiter::HTTP::Content::Language::ENGLISH;
	content->type = &Jupiter::HTTP::Content::Type::Text::HTML;
	content->charset = &Jupiter::HTTP::Content::Type::Text::Charset::UTF8;
	server.hook(RenX_Ladder_WebPlugin::web_hostname, RenX_Ladder_WebPlugin::web_path, content);

	content = new Jupiter::HTTP::Server::Content(RenX_Ladder_WebPlugin::search_page_name, handle_search_page);
	content->language = &Jupiter::HTTP::Content::Language::ENGLISH;
	content->type = &Jupiter::HTTP::Content::Type::Text::HTML;
	content->charset = &Jupiter::HTTP::Content::Type::Text::Charset::UTF8;
	server.hook(RenX_Ladder_WebPlugin::web_hostname, RenX_Ladder_WebPlugin::web_path, content);

	content = new Jupiter::HTTP::Server::Content(RenX_Ladder_WebPlugin::profile_page_name, handle_profile_page);
	content->language = &Jupiter::HTTP::Content::Language::ENGLISH;
	content->type = &Jupiter::HTTP::Content::Type::Text::HTML;
	content->charset = &Jupiter::HTTP::Content::Type::Text::Charset::UTF8;
	server.hook(RenX_Ladder_WebPlugin::web_hostname, RenX_Ladder_WebPlugin::web_path, content);

	return true;
}

RenX_Ladder_WebPlugin::~RenX_Ladder_WebPlugin()
{
	Jupiter::HTTP::Server &server = getHTTPServer();
	server.remove(RenX_Ladder_WebPlugin::web_hostname, RenX_Ladder_WebPlugin::web_path, RenX_Ladder_WebPlugin::ladder_page_name);
	server.remove(RenX_Ladder_WebPlugin::web_hostname, RenX_Ladder_WebPlugin::web_path, RenX_Ladder_WebPlugin::search_page_name);
	server.remove(RenX_Ladder_WebPlugin::web_hostname, RenX_Ladder_WebPlugin::web_path, RenX_Ladder_WebPlugin::profile_page_name);
}

void RenX_Ladder_WebPlugin::init()
{
	FILE *file;
	int chr;

	RenX_Ladder_WebPlugin::web_header_filename = static_cast<std::string>(this->config.get("HeaderFilename"_jrs, "RenX.Ladder.Web.Header.html"_jrs));
	RenX_Ladder_WebPlugin::web_footer_filename = static_cast<std::string>(this->config.get("FooterFilename"_jrs, "RenX.Ladder.Web.Footer.html"_jrs));
	RenX_Ladder_WebPlugin::web_profile_filename = static_cast<std::string>(this->config.get("ProfileFilename"_jrs, "RenX.Ladder.Web.Profile.html"_jrs));
	RenX_Ladder_WebPlugin::web_ladder_table_header_filename = static_cast<std::string>(this->config.get("LadderTableHeaderFilename"_jrs, "RenX.Ladder.Web.Ladder.Table.Header.html"_jrs));
	RenX_Ladder_WebPlugin::web_ladder_table_footer_filename = static_cast<std::string>(this->config.get("LadderTableFooterFilename"_jrs, "RenX.Ladder.Web.Ladder.Table.Footer.html"_jrs));
	RenX_Ladder_WebPlugin::entries_per_page = this->config.get<size_t>("EntriesPerPage"_jrs, 50);
	RenX_Ladder_WebPlugin::min_search_name_length = this->config.get<size_t>("MinSearchNameLength"_jrs, 3);

	RenX_Ladder_WebPlugin::entry_table_row = this->config.get("EntryTableRow"_jrs, R"html(<tr><td class="data-col-a">{RANK}</td><td class="data-col-b"><a href="profile?id={STEAM}&database={OBJECT}">{NAME}</a></td><td class="data-col-a">{SCORE}</td><td class="data-col-b">{SPM}</td><td class="data-col-a">{GAMES}</td><td class="data-col-b">{WINS}</td><td class="data-col-a">{LOSSES}</td><td class="data-col-b">{WLR}</td><td class="data-col-a">{KILLS}</td><td class="data-col-b">{DEATHS}</td><td class="data-col-a">{KDR}</td></tr>)html"_jrs);
	RenX_Ladder_WebPlugin::entry_profile_previous = this->config.get("EntryProfilePrevious"_jrs, R"html(<form class="profile-previous"><input type="hidden" name="database" value="{OBJECT}"/><input type="hidden" name="id" value="{WEAPON}"/><input class="profile-previous-submit" type="submit" value="&#x21A9 Previous" /></form>)html"_jrs);
	RenX_Ladder_WebPlugin::entry_profile_next = this->config.get("EntryProfileNext"_jrs, R"html(<form class="profile-next"><input type="hidden" name="database" value="{OBJECT}"/><input type="hidden" name="id" value="{VSTEAM}"/><input class="profile-next-submit" type="submit" value="Next &#x21AA" /></form>)html"_jrs);

	RenX::sanitizeTags(RenX_Ladder_WebPlugin::entry_table_row);
	RenX::sanitizeTags(RenX_Ladder_WebPlugin::entry_profile_previous);
	RenX::sanitizeTags(RenX_Ladder_WebPlugin::entry_profile_next);

	RenX_Ladder_WebPlugin::header.erase();
	RenX_Ladder_WebPlugin::footer.erase();
	RenX_Ladder_WebPlugin::entry_profile.erase();
	RenX_Ladder_WebPlugin::ladder_table_header.erase();
	RenX_Ladder_WebPlugin::ladder_table_footer.erase();

	/** Load header */
	if (!RenX_Ladder_WebPlugin::web_header_filename.empty())
	{
		file = fopen(RenX_Ladder_WebPlugin::web_header_filename.c_str(), "rb");
		if (file != nullptr)
		{
			while ((chr = fgetc(file)) != EOF)
				RenX_Ladder_WebPlugin::header += chr;
			fclose(file);
		}
	}

	/** Load footer */
	if (!RenX_Ladder_WebPlugin::web_footer_filename.empty())
	{
		file = fopen(RenX_Ladder_WebPlugin::web_footer_filename.c_str(), "rb");
		if (file != nullptr)
		{
			while ((chr = fgetc(file)) != EOF)
				RenX_Ladder_WebPlugin::footer += chr;
			fclose(file);
		}
	}

	/** Load profile */
	if (!RenX_Ladder_WebPlugin::web_profile_filename.empty())
	{
		file = fopen(RenX_Ladder_WebPlugin::web_profile_filename.c_str(), "rb");
		if (file != nullptr)
		{
			while ((chr = fgetc(file)) != EOF)
				RenX_Ladder_WebPlugin::entry_profile += chr;
			RenX::sanitizeTags(RenX_Ladder_WebPlugin::entry_profile);
			fclose(file);
		}
	}

	/** Load table header */
	if (!RenX_Ladder_WebPlugin::web_ladder_table_header_filename.empty())
	{
		file = fopen(RenX_Ladder_WebPlugin::web_ladder_table_header_filename.c_str(), "rb");
		if (file != nullptr)
		{
			while ((chr = fgetc(file)) != EOF)
				RenX_Ladder_WebPlugin::ladder_table_header += chr;
			fclose(file);
		}
	}

	/** Load table footer */
	if (!RenX_Ladder_WebPlugin::web_ladder_table_footer_filename.empty())
	{
		file = fopen(RenX_Ladder_WebPlugin::web_ladder_table_footer_filename.c_str(), "rb");
		if (file != nullptr)
		{
			while ((chr = fgetc(file)) != EOF)
				RenX_Ladder_WebPlugin::ladder_table_footer += chr;
			fclose(file);
		}
	}
}

int RenX_Ladder_WebPlugin::OnRehash()
{
	RenX::Plugin::OnRehash();
	this->init();
	return 0;
}

// Plugin instantiation and entry point.
RenX_Ladder_WebPlugin pluginInstance;

/** Search bar */
Jupiter::String generate_search(RenX::LadderDatabase *db)
{
	Jupiter::String result(256);

	result = R"database-search(<form action="search" method="get" class="leaderboard-search"><input type="text" class="leaderboard-search-input" name="name" size="30" placeholder="Player name" value=""/>)database-search"_jrs;

	if (db != nullptr && db != RenX::default_ladder_database)
	{
		result += R"database-search(<input type="hidden" name="database" value=")database-search"_jrs;
		result += db->getName();
		result += R"database-search("/>)database-search"_jrs;
	}
	result += R"database-search(<input type="submit"  class="leaderboard-button" value="Search"/></form>)database-search"_jrs;
	return result;
}

/** Database selector */
Jupiter::String generate_database_selector(RenX::LadderDatabase *db, const Jupiter::HTTP::HTMLFormResponse::TableType &query_params)
{
	RenX::LadderDatabase *db_ptr;
	Jupiter::String result(256);

	result = R"database-select(<form method="get" class="database-select-form"><select name="database" class="database-select">)database-select"_jrs;
	if (db != nullptr)
	{
		result += "<option value=\""_jrs;
		result += db->getName();
		result += "\">"_jrs;
		result += db->getName();
		result += "</option>"_jrs;
	}
	else if (RenX::ladder_databases.size() == 0)
		return Jupiter::String::empty;

	for (size_t index = 0; index != RenX::ladder_databases.size(); ++index)
	{
		db_ptr = RenX::ladder_databases.get(index);
		if (db_ptr != db)
		{
			db_ptr = RenX::ladder_databases.get(index);
			result += "<option value=\""_jrs;
			result += db_ptr->getName();
			result += "\">"_jrs;
			result += db_ptr->getName();
			result += "</option>"_jrs;
		}
	}

	auto value = query_params.find("id"_jrs);
	if (value != query_params.end())
	{
		result += R"html(<input type="hidden" name="id" value=")html"_jrs;
		result += value->second;
		result += R"html("/>)html"_jrs;
	}

	result += R"database-select(</select><input type="submit" class="leaderboard-button" value="Go"/></form>)database-select"_jrs;
	return result;
}

/** Page buttons */
Jupiter::String generate_page_buttons(RenX::LadderDatabase *db)
{
	Jupiter::String result(256);
	size_t entry_count = db->getEntries();
	size_t entries_per_page = pluginInstance.getEntriesPerPage();

	result = R"html(<div id="leaderboard-paging">)html"_jrs;

	size_t entry_index = 0, page_index = 1;
	while (entry_index < entry_count)
	{
		// Add page
		result += R"html(<span class="leaderboard-page"><a href="?start=)html"_jrs;
		result += Jupiter::StringS::Format("%u", entry_index);
		if (db != RenX::default_ladder_database)
		{
			result += "&database="_jrs;
			result += db->getName();
		}
		result += R"html(">)html"_jrs;
		result += Jupiter::StringS::Format("%u", page_index);
		result += R"html(</a></span>)html"_jrs;

		// Increment indexes
		entry_index += entries_per_page;
		++page_index;
	}

	result += R"html(</div>)html"_jrs;
	return result;
}

/** Ladder page */

Jupiter::String RenX_Ladder_WebPlugin::generate_entry_table(RenX::LadderDatabase *db, uint8_t format, size_t index, size_t count)
{
	if (db->getEntries() == 0) // No ladder data
		return Jupiter::String("Error: No ladder data"_jrs);

	if (index >= db->getEntries() || count == 0) // Invalid entry range
		return Jupiter::String("Error: Invalid range"_jrs);

	if (index + count > db->getEntries()) // Invalid entry range; use valid portion of range
		count = db->getEntries() - index;

	RenX::LadderDatabase::Entry *node = db->getHead();

	// iterate to requested index
	while (index != 0)
	{
		node = node->next;
		--index;
	}

	// table header
	Jupiter::String result(2048);

	if ((format & this->FLAG_INCLUDE_DATA_HEADER) != 0) // Data Header
		result = RenX_Ladder_WebPlugin::ladder_table_header;

	// append rows
	Jupiter::String row(256);
	while (count != 0)
	{
		row = RenX_Ladder_WebPlugin::entry_table_row;
		row.replace(RenX::tags->INTERNAL_OBJECT_TAG, db->getName());
		RenX::processTags(row, *node);
		result += row;
		node = node->next;
		--count;
	}

	if ((format & this->FLAG_INCLUDE_DATA_FOOTER) != 0) // Data footer
	result += RenX_Ladder_WebPlugin::ladder_table_footer;

	// search buttons
	result += generate_page_buttons(db);

	return result;
}

Jupiter::String *RenX_Ladder_WebPlugin::generate_ladder_page(RenX::LadderDatabase *db, uint8_t format, size_t index, size_t count, const Jupiter::HTTP::HTMLFormResponse::TableType &query_params)
{
	Jupiter::String *result = new Jupiter::String(2048);

	if ((format & this->FLAG_INCLUDE_PAGE_HEADER) != 0) // Header
		result->concat(RenX_Ladder_WebPlugin::header);

	if ((format & this->FLAG_INCLUDE_SEARCH) != 0) // Search
		result->concat(generate_search(db));

	if ((format & this->FLAG_INCLUDE_SELECTOR) != 0) // Selector
		result->concat(generate_database_selector(db, query_params));

	result->concat(this->generate_entry_table(db, format, index, count));

	if ((format & this->FLAG_INCLUDE_PAGE_FOOTER) != 0) // Footer
		result->concat(RenX_Ladder_WebPlugin::footer);

	return result;
}

// format:
//	include_header | include_footer | include_any_headers | include_any_footers

/** Search page */
Jupiter::String *RenX_Ladder_WebPlugin::generate_search_page(RenX::LadderDatabase *db, uint8_t format, size_t start_index, size_t count, const Jupiter::ReadableString &name, const Jupiter::HTTP::HTMLFormResponse::TableType &query_params)
{
	Jupiter::String *result = new Jupiter::String(2048);

	if ((format & this->FLAG_INCLUDE_PAGE_HEADER) != 0) // Header
		result->concat(RenX_Ladder_WebPlugin::header);

	if ((format & this->FLAG_INCLUDE_SEARCH) != 0) // Search
		result->concat(generate_search(db));

	if ((format & this->FLAG_INCLUDE_SELECTOR) != 0) // Selector
		result->concat(generate_database_selector(db, query_params));

	if (db->getEntries() == 0) // No ladder data
	{
		result->concat("Error: No ladder data"_jrs);

		if ((format & this->FLAG_INCLUDE_PAGE_FOOTER) != 0) // Footer
			result->concat(RenX_Ladder_WebPlugin::footer);

		return result;
	}

	if ((format & this->FLAG_INCLUDE_DATA_HEADER) != 0) // Data header
		result->concat(RenX_Ladder_WebPlugin::ladder_table_header);

	// append rows
	Jupiter::String row(256);
	RenX::LadderDatabase::Entry *node = db->getHead();
	while (node != nullptr)
	{
		if (node->most_recent_name.findi(name) != Jupiter::INVALID_INDEX) // match found
		{
			row = RenX_Ladder_WebPlugin::entry_table_row;
			row.replace(RenX::tags->INTERNAL_OBJECT_TAG, db->getName());
			RenX::processTags(row, *node);
			result->concat(row);
		}
		node = node->next;
	}
	
	if ((format & this->FLAG_INCLUDE_DATA_FOOTER) != 0) // Data footer
		result->concat(RenX_Ladder_WebPlugin::ladder_table_footer);

	if ((format & this->FLAG_INCLUDE_PAGE_FOOTER) != 0) // Footer
		result->concat(RenX_Ladder_WebPlugin::footer);

	return result;
}

/** Profile page */
Jupiter::String *RenX_Ladder_WebPlugin::generate_profile_page(RenX::LadderDatabase *db, uint8_t format, uint64_t steam_id, const Jupiter::HTTP::HTMLFormResponse::TableType &query_params)
{
	Jupiter::String *result = new Jupiter::String(2048);

	if ((format & this->FLAG_INCLUDE_PAGE_HEADER) != 0)
		result->concat(RenX_Ladder_WebPlugin::header);

	if ((format & this->FLAG_INCLUDE_SEARCH) != 0) // Search
		result->concat(generate_search(db));

	if ((format & this->FLAG_INCLUDE_SELECTOR) != 0) // Selector
		result->concat(generate_database_selector(db, query_params));

	if (db->getEntries() == 0) // No ladder data
	{
		result->concat("Error: No ladder data"_jrs);

		if ((format & this->FLAG_INCLUDE_PAGE_FOOTER) != 0) // Footer
			result->concat(RenX_Ladder_WebPlugin::footer);

		return result;
	}

	RenX::LadderDatabase::Entry *entry = db->getHead();
	while (entry != nullptr)
	{
		if (entry->steam_id == steam_id) // match found
			break;
		entry = entry->next;
	}

	if (entry == nullptr)
		result->concat("Error: Player not found"_jrs);
	else
	{
		Jupiter::String profile_data(RenX_Ladder_WebPlugin::entry_profile);
		RenX::processTags(profile_data, *entry);
		result->concat(profile_data);

		result->concat("<div class=\"profile-navigation\">"_jrs);
		if (entry->prev != nullptr)
		{
			profile_data = RenX_Ladder_WebPlugin::entry_profile_previous;
			profile_data.replace(RenX::tags->INTERNAL_OBJECT_TAG, db->getName());
			profile_data.replace(RenX::tags->INTERNAL_WEAPON_TAG, Jupiter::StringS::Format("%llu", entry->prev->steam_id));
			RenX::processTags(profile_data, *entry->prev);
			result->concat(profile_data);
		}
		if (entry->next != nullptr)
		{
			profile_data = RenX_Ladder_WebPlugin::entry_profile_next;
			profile_data.replace(RenX::tags->INTERNAL_OBJECT_TAG, db->getName());
			profile_data.replace(RenX::tags->INTERNAL_VICTIM_STEAM_TAG, Jupiter::StringS::Format("%llu", entry->next->steam_id));
			RenX::processTags(profile_data, *entry->next);
			result->concat(profile_data);
		}
		result->concat("</div>"_jrs);
	}

	if ((format & this->FLAG_INCLUDE_PAGE_FOOTER) != 0) // Footer
		result->concat(RenX_Ladder_WebPlugin::footer);

	return result;
}

/** Content functions */

Jupiter::ReadableString *generate_no_db_page(const Jupiter::HTTP::HTMLFormResponse::TableType &query_params)
{
	Jupiter::String *result = new Jupiter::String(pluginInstance.header);
	if (RenX::ladder_databases.size() != 0)
	{
		result->concat(generate_search(nullptr));
		result->concat(generate_database_selector(nullptr, query_params));
		result->concat("Error: No such database exists"_jrs);
	}
	else
		result->concat("Error: No ladder databases loaded"_jrs);
	result->concat(pluginInstance.footer);
	return result;
}

Jupiter::ReadableString *handle_ladder_page(const Jupiter::ReadableString &query_string)
{
	Jupiter::HTTP::HTMLFormResponse html_form_response(query_string);
	RenX::LadderDatabase *db = RenX::default_ladder_database;
	size_t start_index = 0, count = pluginInstance.getEntriesPerPage();
	uint8_t format = 0xFF;

	if (html_form_response.table.size() != 0)
	{
		format = html_form_response.tableGetCast<uint8_t>("format"_jrs, format);
		start_index = html_form_response.tableGetCast<size_t>("start"_jrs, start_index);
		count = html_form_response.tableGetCast<size_t>("count"_jrs, count);
		
		const Jupiter::ReadableString &db_name = html_form_response.tableGet("database"_jrs, Jupiter::ReferenceString::empty);
		if (db_name.isNotEmpty())
		{
			db = nullptr;
			for (size_t index = 0; index != RenX::ladder_databases.size(); ++index)
				if (RenX::ladder_databases.get(index)->getName().equalsi(db_name))
				{
					db = RenX::ladder_databases.get(index);
					break;
				}
		}
	}

	if (db == nullptr)
		return generate_no_db_page(html_form_response.table);

	return pluginInstance.generate_ladder_page(db, format, start_index, count, html_form_response.table);
}

Jupiter::ReadableString *handle_search_page(const Jupiter::ReadableString &query_string)
{
	Jupiter::HTTP::HTMLFormResponse html_form_response(query_string);
	RenX::LadderDatabase *db = RenX::default_ladder_database;
	uint8_t format = 0xFF;
	size_t start_index = 0, count = pluginInstance.getEntriesPerPage();
	Jupiter::ReferenceString name;

	if (html_form_response.table.size() != 0)
	{
		format = html_form_response.tableGetCast<uint8_t>("format"_jrs, format);
		start_index = html_form_response.tableGetCast<size_t>("start"_jrs, start_index);
		count = html_form_response.tableGetCast<size_t>("count"_jrs, count);
		name = html_form_response.tableGet("name"_jrs, name);

		const Jupiter::ReadableString &db_name = html_form_response.tableGet("database"_jrs, Jupiter::ReferenceString::empty);
		if (db_name.isNotEmpty())
		{
			db = nullptr;
			for (size_t index = 0; index != RenX::ladder_databases.size(); ++index)
				if (RenX::ladder_databases.get(index)->getName().equalsi(db_name))
				{
					db = RenX::ladder_databases.get(index);
					break;
				}
		}
	}

	if (db == nullptr)
		return generate_no_db_page(html_form_response.table);

	if (name.size() < pluginInstance.getMinSearchNameLength()) // Generate ladder page when no name specified
		return handle_ladder_page(query_string);

	return pluginInstance.generate_search_page(db, format, start_index, count, name, html_form_response.table);
}

Jupiter::ReadableString *handle_profile_page(const Jupiter::ReadableString &query_string)
{
	Jupiter::HTTP::HTMLFormResponse html_form_response(query_string);
	RenX::LadderDatabase *db = RenX::default_ladder_database;
	uint64_t steam_id = 0;
	uint8_t format = 0xFF;

	if (html_form_response.table.size() != 0)
	{
		format = html_form_response.tableGetCast<uint8_t>("format"_jrs, format);
		steam_id = html_form_response.tableGetCast<uint64_t>("id"_jrs, steam_id);

		const Jupiter::ReadableString &db_name = html_form_response.tableGet("database"_jrs, Jupiter::ReferenceString::empty);
		if (db_name.isNotEmpty())
		{
			db = nullptr;
			for (size_t index = 0; index != RenX::ladder_databases.size(); ++index)
				if (RenX::ladder_databases.get(index)->getName().equalsi(db_name))
				{
					db = RenX::ladder_databases.get(index);
					break;
				}
		}
	}

	if (db == nullptr)
		return generate_no_db_page(html_form_response.table);

	return pluginInstance.generate_profile_page(db, format, steam_id, html_form_response.table);
}

extern "C" JUPITER_EXPORT Jupiter::Plugin *getPlugin()
{
	return &pluginInstance;
}
