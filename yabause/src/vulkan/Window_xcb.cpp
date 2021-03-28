/* -----------------------------------------------------
This source code is public domain ( CC0 )
The code is provided as-is without limitations, requirements and responsibilities.
Creators and contributors to this source code are provided as a token of appreciation
and no one associated with this source code can be held responsible for any possible
damages or losses of any kind.

Original file creator:  Niko Kauppi (Code maintenance)
Contributors:
hodasemi (XCB validation)
----------------------------------------------------- */

#include "BUILD_OPTIONS.h"
#include "Platform.h"

#include "Window.h"
#include "Shared.h"
#include "Renderer.h"

#include <assert.h>
#include <iostream>

#if VK_USE_PLATFORM_XCB_KHR

void Window::_InitOSWindow()
{
	// create connection to X11 server
	const xcb_setup_t		*	setup		= nullptr;
	xcb_screen_iterator_t		iter;
	int							screen		= 0;

	_xcb_connection			=	xcb_connect( nullptr, &screen );
	if( _xcb_connection == nullptr ) {
		std::cout << "Cannot find a compatible Vulkan ICD.\n";
		exit( -1 );
	}

	setup		= xcb_get_setup( _xcb_connection );
	iter		= xcb_setup_roots_iterator( setup );
	while( screen-- > 0 ) {
		xcb_screen_next( &iter );
	}
	_xcb_screen = iter.data;

	// create window
	VkRect2D dimensions = { { 0, 0 }, { _surface_size_x, _surface_size_y } };

	assert( dimensions.extent.width > 0 );
	assert( dimensions.extent.height > 0 );

	uint32_t value_mask, value_list[ 32 ];

	_xcb_window = xcb_generate_id( _xcb_connection );

	value_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
	value_list[ 0 ] = _xcb_screen->black_pixel;
	value_list[ 1 ] = XCB_EVENT_MASK_KEY_RELEASE | XCB_EVENT_MASK_EXPOSURE;

	xcb_create_window( _xcb_connection, XCB_COPY_FROM_PARENT, _xcb_window,
		_xcb_screen->root, dimensions.offset.x, dimensions.offset.y,
		dimensions.extent.width, dimensions.extent.height, 0,
		XCB_WINDOW_CLASS_INPUT_OUTPUT, _xcb_screen->root_visual,
		value_mask, value_list );

	/* Magic code that will send notification when window is destroyed */
	xcb_intern_atom_cookie_t cookie =
		xcb_intern_atom( _xcb_connection, 1, 12, "WM_PROTOCOLS" );
	xcb_intern_atom_reply_t *reply =
		xcb_intern_atom_reply( _xcb_connection, cookie, 0 );

	xcb_intern_atom_cookie_t cookie2 =
		xcb_intern_atom( _xcb_connection, 0, 16, "WM_DELETE_WINDOW" );
	_xcb_atom_window_reply =
		xcb_intern_atom_reply( _xcb_connection, cookie2, 0 );

	xcb_change_property( _xcb_connection, XCB_PROP_MODE_REPLACE, _xcb_window,
		( *reply ).atom, 4, 32, 1,
		&( *_xcb_atom_window_reply ).atom );
	free( reply );

	xcb_map_window( _xcb_connection, _xcb_window );

	// Force the x/y coordinates to 100,100 results are identical in consecutive
	// runs
	const uint32_t coords[] = { 100, 100 };
	xcb_configure_window( _xcb_connection, _xcb_window,
		XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y, coords );
	xcb_flush( _xcb_connection );

	/*
	xcb_generic_event_t *e;
	while( ( e = xcb_wait_for_event( _xcb_connection ) ) ) {
		if( ( e->response_type & ~0x80 ) == XCB_EXPOSE )
			break;
	}
	*/
}

void Window::_DeInitOSWindow()
{
	xcb_destroy_window( _xcb_connection, _xcb_window );
	xcb_disconnect( _xcb_connection );
	_xcb_window			= 0;
	_xcb_connection		= nullptr;
}

void Window::_UpdateOSWindow()
{
	auto event = xcb_poll_for_event( _xcb_connection );

	// if there is no event, event will be NULL
	// need to check for event == NULL to prevent segfault
	if( !event )
		return;

	switch( event->response_type & ~0x80 ) {
	case XCB_CLIENT_MESSAGE:
		if( ( (xcb_client_message_event_t*)event )->data.data32[ 0 ] == _xcb_atom_window_reply->atom ) {
			Close();
		}
		break;
	default:
		break;
	}
	free( event );
}

void Window::_InitOSSurface()
{
	VkXcbSurfaceCreateInfoKHR create_info {};
	create_info.sType			= VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
	create_info.connection		= _xcb_connection;
	create_info.window			= _xcb_window;
    ErrorCheck( vkCreateXcbSurfaceKHR( _renderer->GetVulkanInstance(), &create_info, nullptr, &_surface ) );
}

#endif
