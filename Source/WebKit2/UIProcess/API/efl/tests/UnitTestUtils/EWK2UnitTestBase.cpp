/*
    Copyright (C) 2012 Samsung Electronics
    Copyright (C) 2012 Intel Corporation. All rights reserved.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this library; if not, write to the Free Software Foundation,
    Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/

#include "config.h"
#include "EWK2UnitTestBase.h"

#include "EWK2UnitTestEnvironment.h"
#include <EWebKit2.h>
#include <Ecore.h>
#include <glib-object.h>
#include <wtf/UnusedParam.h>

extern EWK2UnitTest::EWK2UnitTestEnvironment* environment;

namespace EWK2UnitTest {

static void onLoadFinished(void* userData, Evas_Object* webView, void* eventInfo)
{
    UNUSED_PARAM(webView);
    UNUSED_PARAM(eventInfo);

    bool* loadFinished = static_cast<bool*>(userData);
    *loadFinished = true;
}

EWK2UnitTestBase::EWK2UnitTestBase()
    : m_ecoreEvas(0)
    , m_webView(0)
{
}

void EWK2UnitTestBase::SetUp()
{
    ASSERT_GT(ecore_evas_init(), 0);

    // glib support (for libsoup).
    g_type_init();
    if (!ecore_main_loop_glib_integrate())
        fprintf(stderr, "WARNING: Glib main loop integration is not working. Some tests may fail.");

    unsigned int width = environment->defaultWidth();
    unsigned int height = environment->defaultHeight();

    if (environment->useX11Window())
        m_ecoreEvas = ecore_evas_new(0, 0, 0, width, height, 0);
    else
        m_ecoreEvas = ecore_evas_buffer_new(width, height);

    ecore_evas_show(m_ecoreEvas);
    Evas* evas = ecore_evas_get(m_ecoreEvas);

    m_webView = ewk_view_add(evas);
    ewk_view_theme_set(m_webView, environment->defaultTheme());

    evas_object_resize(m_webView, width, height);
    evas_object_show(m_webView);
    evas_object_focus_set(m_webView, true);
}

void EWK2UnitTestBase::TearDown()
{
    evas_object_del(m_webView);
    ecore_evas_free(m_ecoreEvas);
    ecore_evas_shutdown();
}

void EWK2UnitTestBase::loadUrlSync(const char* url)
{
    bool loadFinished = false;

    evas_object_smart_callback_add(m_webView, "load,finished", onLoadFinished, &loadFinished);
    ewk_view_uri_set(m_webView, url);

    while (!loadFinished)
        ecore_main_loop_iterate();

    evas_object_smart_callback_del(m_webView, "load,finished", onLoadFinished);
}

} // namespace EWK2UnitTest
