// license:BSD-3-Clause
// copyright-holders:Aaron Giles, Vas Crabb
//============================================================
//
//	disasmbasewininfo.c - Win32 debug window handling
//
//============================================================

#include "disasmbasewininfo.h"

#include "debugviewinfo.h"
#include "disasmviewinfo.h"

#include "debugger.h"
#include "debug/debugcon.h"
#include "debug/debugcpu.h"

//#include "winutf8.h"


disasmbasewin_info::disasmbasewin_info(debugger_windows_interface &debugger, bool is_main_console, LPCSTR title, WNDPROC handler) :
	editwin_info(debugger, is_main_console, title, handler)
{
	if (!window())
		return;

	m_views[0].reset(global_alloc(disasmview_info(debugger, *this, window())));
	if ((m_views[0] == NULL) || !m_views[0]->is_valid())
	{
		m_views[0].reset();
		return;
	}

	// create the options menu
	HMENU const optionsmenu = CreatePopupMenu();
	AppendMenu(optionsmenu, MF_ENABLED, ID_TOGGLE_BREAKPOINT, TEXT("Toggle breakpoint at cursor\tF9"));
	AppendMenu(optionsmenu, MF_ENABLED, ID_RUN_TO_CURSOR, TEXT("Run to cursor\tF4"));
	AppendMenu(optionsmenu, MF_DISABLED | MF_SEPARATOR, 0, TEXT(""));
	AppendMenu(optionsmenu, MF_ENABLED, ID_SHOW_RAW, TEXT("Raw opcodes\tCtrl+R"));
	AppendMenu(optionsmenu, MF_ENABLED, ID_SHOW_ENCRYPTED, TEXT("Encrypted opcodes\tCtrl+E"));
	AppendMenu(optionsmenu, MF_ENABLED, ID_SHOW_COMMENTS, TEXT("Comments\tCtrl+M"));
	AppendMenu(GetMenu(window()), MF_ENABLED | MF_POPUP, (UINT_PTR)optionsmenu, TEXT("Options"));

	// set up the view to track the initial expression
	downcast<disasmview_info *>(m_views[0].get())->set_expression("curpc");
	m_views[0]->set_source_for_visible_cpu();
}


disasmbasewin_info::~disasmbasewin_info()
{
}


bool disasmbasewin_info::handle_key(WPARAM wparam, LPARAM lparam)
{
	if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
	{
		switch (wparam)
		{
		case 'R':
			SendMessage(window(), WM_COMMAND, ID_SHOW_RAW, 0);
			return 1;

		case 'E':
			SendMessage(window(), WM_COMMAND, ID_SHOW_ENCRYPTED, 0);
			return 1;

		case 'N':
			SendMessage(window(), WM_COMMAND, ID_SHOW_COMMENTS, 0);
			return 1;
		}
	}

	switch (wparam)
	{
	/* ajg - steals the F4 from the global key handler - but ALT+F4 didn't work anyways ;) */
	case VK_F4:
		SendMessage(window(), WM_COMMAND, ID_RUN_TO_CURSOR, 0);
		return 1;

	case VK_F9:
		SendMessage(window(), WM_COMMAND, ID_TOGGLE_BREAKPOINT, 0);
		return 1;

	case VK_RETURN:
		if (m_views[0]->cursor_visible())
		{
			SendMessage(window(), WM_COMMAND, ID_STEP, 0);
			return 1;
		}
		break;
	}

	return editwin_info::handle_key(wparam, lparam);
}


void disasmbasewin_info::update_menu()
{
	editwin_info::update_menu();

	HMENU const menu = GetMenu(window());

	bool const disasm_cursor_visible = m_views[0]->cursor_visible();
	EnableMenuItem(menu, ID_TOGGLE_BREAKPOINT, MF_BYCOMMAND | (disasm_cursor_visible ? MF_ENABLED : MF_GRAYED));
	EnableMenuItem(menu, ID_RUN_TO_CURSOR, MF_BYCOMMAND | (disasm_cursor_visible ? MF_ENABLED : MF_GRAYED));

	disasm_right_column const rightcol = downcast<disasmview_info *>(m_views[0].get())->right_column();
	CheckMenuItem(menu, ID_SHOW_RAW, MF_BYCOMMAND | (rightcol == DASM_RIGHTCOL_RAW ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(menu, ID_SHOW_ENCRYPTED, MF_BYCOMMAND | (rightcol == DASM_RIGHTCOL_ENCRYPTED ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(menu, ID_SHOW_COMMENTS, MF_BYCOMMAND | (rightcol == DASM_RIGHTCOL_COMMENTS ? MF_CHECKED : MF_UNCHECKED));
}


bool disasmbasewin_info::handle_command(WPARAM wparam, LPARAM lparam)
{
	disasmview_info *const dasmview = downcast<disasmview_info *>(m_views[0].get());

	switch (HIWORD(wparam))
	{
	// menu selections
	case 0:
		switch (LOWORD(wparam))
		{
		case ID_SHOW_RAW:
			dasmview->set_right_column(DASM_RIGHTCOL_RAW);
			recompute_children();
			return true;

		case ID_SHOW_ENCRYPTED:
			dasmview->set_right_column(DASM_RIGHTCOL_ENCRYPTED);
			recompute_children();
			return true;

		case ID_SHOW_COMMENTS:
			dasmview->set_right_column(DASM_RIGHTCOL_COMMENTS);
			recompute_children();
			return true;

		case ID_RUN_TO_CURSOR:
			if (dasmview->cursor_visible())
			{
				offs_t const address = dasmview->selected_address();
				if (dasmview->source_is_visible_cpu())
				{
					astring command;
					command.printf("go 0x%X", address);
					debug_console_execute_command(machine(), command, 1);
				}
				else
				{
					dasmview->source_device()->debug()->go(address);
				}
			}
			return true;

		case ID_TOGGLE_BREAKPOINT:
			if (dasmview->cursor_visible())
			{
				offs_t const address = dasmview->selected_address();
				device_debug *const debug = dasmview->source_device()->debug();
				INT32 bpindex = -1;

				/* first find an existing breakpoint at this address */
				for (device_debug::breakpoint *bp = debug->breakpoint_first(); bp != NULL; bp = bp->next())
				{
					if (address == bp->address())
					{
						bpindex = bp->index();
						break;
					}
				}

				/* if it doesn't exist, add a new one */
				if (dasmview->source_is_visible_cpu())
				{
					astring command;
					if (bpindex == -1)
						command.printf("bpset 0x%X", address);
					else
						command.printf("bpclear 0x%X", bpindex);
					debug_console_execute_command(machine(), command, 1);
				}
				else
				{
					if (bpindex == -1)
					{
						bpindex = debug->breakpoint_set(address, NULL, NULL);
						debug_console_printf(machine(), "Breakpoint %X set\n", bpindex);
					}
					else
					{
						debug->breakpoint_clear(bpindex);
						debug_console_printf(machine(), "Breakpoint %X cleared\n", bpindex);
					}
					machine().debug_view().update_all();
					debugger_refresh_display(machine());
				}
			}
			return true;
		}
		break;
	}
	return editwin_info::handle_command(wparam, lparam);
}
