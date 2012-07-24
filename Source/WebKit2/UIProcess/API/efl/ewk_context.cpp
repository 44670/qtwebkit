/*
 * Copyright (C) 2012 Samsung Electronics
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this program; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include "config.h"
#include "ewk_context.h"

#include "BatteryProvider.h"
#include "WKAPICast.h"
#include "WKContextSoup.h"
#include "WKRetainPtr.h"
#include "WKString.h"
#include "ewk_context_private.h"
#include "ewk_context_request_manager_client_private.h"
#include "ewk_cookie_manager_private.h"
#include <wtf/HashMap.h>
#include <wtf/text/WTFString.h>

using namespace WebKit;

struct _Ewk_Url_Scheme_Handler {
    Ewk_Url_Scheme_Request_Cb callback;
    void* userData;

    _Ewk_Url_Scheme_Handler()
        : callback(0)
        , userData(0)
    { }

    _Ewk_Url_Scheme_Handler(Ewk_Url_Scheme_Request_Cb callback, void* userData)
        : callback(callback)
        , userData(userData)
    { }
};

typedef HashMap<String, _Ewk_Url_Scheme_Handler> URLSchemeHandlerMap;

struct _Ewk_Context {
    WKRetainPtr<WKContextRef> context;

    Ewk_Cookie_Manager* cookieManager;
#if ENABLE(BATTERY_STATUS)
    RefPtr<BatteryProvider> batteryProvider;
#endif

    WKRetainPtr<WKSoupRequestManagerRef> requestManager;
    URLSchemeHandlerMap urlSchemeHandlers;

    _Ewk_Context(WKContextRef contextRef)
        : context(contextRef)
        , cookieManager(0)
        , requestManager(WKContextGetSoupRequestManager(contextRef))
    {
#if ENABLE(BATTERY_STATUS)
        WKBatteryManagerRef wkBatteryManager = WKContextGetBatteryManager(contextRef);
        batteryProvider = BatteryProvider::create(wkBatteryManager);
#endif

        ewk_context_request_manager_client_attach(this);
    }

    ~_Ewk_Context()
    {
        if (cookieManager)
            ewk_cookie_manager_free(cookieManager);
    }
};

Ewk_Cookie_Manager* ewk_context_cookie_manager_get(const Ewk_Context* ewkContext)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(ewkContext, 0);

    if (!ewkContext->cookieManager)
        const_cast<Ewk_Context*>(ewkContext)->cookieManager = ewk_cookie_manager_new(WKContextGetCookieManager(ewkContext->context.get()));

    return ewkContext->cookieManager;
}

WKContextRef ewk_context_WKContext_get(const Ewk_Context* ewkContext)
{
    return ewkContext->context.get();
}

/**
 * @internal
 * Retrieve the request manager for @a ewkContext.
 *
 * @param ewkContext a #Ewk_Context object.
 */
WKSoupRequestManagerRef ewk_context_request_manager_get(const Ewk_Context* ewkContext)
{
    return ewkContext->requestManager.get();
}

/**
 * @internal
 * A new URL request was received.
 *
 * @param ewkContext a #Ewk_Context object.
 * @param schemeRequest a #Ewk_Url_Scheme_Request object.
 */
void ewk_context_url_scheme_request_received(Ewk_Context* ewkContext, Ewk_Url_Scheme_Request* schemeRequest)
{
    EINA_SAFETY_ON_NULL_RETURN(ewkContext);
    EINA_SAFETY_ON_NULL_RETURN(schemeRequest);

    _Ewk_Url_Scheme_Handler handler = ewkContext->urlSchemeHandlers.get(ewk_url_scheme_request_scheme_get(schemeRequest));
    if (!handler.callback)
        return;

    handler.callback(schemeRequest, handler.userData);
}

static inline Ewk_Context* createDefaultEwkContext()
{
    return new Ewk_Context(WKContextGetSharedProcessContext());
}

Ewk_Context* ewk_context_default_get()
{
    static Ewk_Context* defaultContext = createDefaultEwkContext();

    return defaultContext;
}

Eina_Bool ewk_context_uri_scheme_register(Ewk_Context* ewkContext, const char* scheme, Ewk_Url_Scheme_Request_Cb callback, void* userData)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(ewkContext, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(scheme, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(callback, false);

    ewkContext->urlSchemeHandlers.set(String::fromUTF8(scheme), _Ewk_Url_Scheme_Handler(callback, userData));
    WKRetainPtr<WKStringRef> wkScheme(AdoptWK, WKStringCreateWithUTF8CString(scheme));
    WKSoupRequestManagerRegisterURIScheme(ewkContext->requestManager.get(), wkScheme.get());

    return true;
}
