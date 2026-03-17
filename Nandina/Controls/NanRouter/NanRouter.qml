pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls

QtObject {
    id: root

    enum NavigationMode {
        Replace,
        Push
    }

    required property StackView stackView
    property list<QtObject> routes
    property string routeIdRole: "key"
    property string sectionRole: "section"
    property string pageSourceRole: "pageSource"
    property string initialPropertiesRole: "initialProperties"
    property bool injectRouterContext: true
    property var currentRouteId: undefined
    property var currentRouteParams: ({})
    // Each entry: { id: routeId, params: {} }
    property var history: []

    // Private: prevents double-push on initialization
    property bool _initialNavigationDone: false

    readonly property var currentRoute: root.route(root.currentRouteId)
    readonly property bool canGoBack: root.history.length > 1

    signal navigated(var route, var params)

    function _routeValue(route, roleName) {
        return route ? route[roleName] : undefined;
    }

    function _routeId(route) {
        return root._routeValue(route, root.routeIdRole);
    }

    function _normalizedParams(params) {
        return params === undefined || params === null ? ({}) : params;
    }

    function _routeProperties(route, params) {
        const baseProperties = root._routeValue(route, root.initialPropertiesRole) || {};
        const routeParams = root._normalizedParams(params);
        const mergedProperties = Object.assign({}, baseProperties);
        if (root.injectRouterContext) {
            mergedProperties.router = root;
            mergedProperties.routeSpec = route;  // full spec object available to the page
        }
        mergedProperties.routeParams = routeParams;
        return mergedProperties;
    }

    function _commitHistory(routeId, params, navigationMode) {
        const entry = {
            id: routeId,
            params: root._normalizedParams(params)
        };
        if (navigationMode === NanRouter.NavigationMode.Push || root.history.length === 0) {
            root.history = root.history.concat([entry]);
            return;
        }
        const updated = root.history.slice();
        updated[updated.length - 1] = entry;
        root.history = updated;
    }

    // Performs the very first navigation so the router (not StackView.initialItem)
    // owns page creation and injects router context correctly.
    function _doInitialNavigation() {
        if (root._initialNavigationDone || !root.stackView || root.currentRouteId === undefined)
            return;
        if (root.stackView.depth === 0) {
            root._initialNavigationDone = true;
            root.navigate(root.currentRouteId, NanRouter.NavigationMode.Replace);
        }
    }

    function firstRouteId() {
        return root.routes.length > 0 ? root._routeId(root.routes[0]) : undefined;
    }

    function route(routeId) {
        for (let index = 0; index < root.routes.length; index++) {
            const candidate = root.routes[index];
            if (root._routeId(candidate) === routeId)
                return candidate;
        }
        return null;
    }

    function routesForSection(section) {
        const groupedRoutes = [];
        for (let index = 0; index < root.routes.length; index++) {
            const candidate = root.routes[index];
            if (root._routeValue(candidate, root.sectionRole) === section)
                groupedRoutes.push(candidate);
        }
        return groupedRoutes;
    }

    function navigate(routeId, navigationMode = NanRouter.NavigationMode.Replace, params = undefined) {
        if (!root.stackView)
            return null;

        const targetRoute = root.route(routeId);
        if (!targetRoute)
            return null;

        const pageSource = root._routeValue(targetRoute, root.pageSourceRole);
        if (!pageSource)
            return null;

        const initialProperties = root._routeProperties(targetRoute, params);

        if (navigationMode === NanRouter.NavigationMode.Push) {
            root.stackView.push(pageSource, initialProperties);
        } else if (root.stackView.depth === 0) {
            root.stackView.push(pageSource, initialProperties, StackView.Immediate);
        } else {
            root.stackView.replace(pageSource, initialProperties, StackView.ReplaceTransition);
        }

        root._commitHistory(routeId, params, navigationMode);
        root.currentRouteId = routeId;
        root.currentRouteParams = root._normalizedParams(params);
        root.navigated(targetRoute, root.currentRouteParams);
        return targetRoute;
    }

    function replace(routeId, params = undefined) {
        return root.navigate(routeId, NanRouter.NavigationMode.Replace, params);
    }

    function push(routeId, params = undefined) {
        return root.navigate(routeId, NanRouter.NavigationMode.Push, params);
    }

    function back() {
        if (!root.canGoBack)
            return false;
        root.stackView.pop();
        const updatedHistory = root.history.slice(0, root.history.length - 1);
        root.history = updatedHistory;
        const prevEntry = updatedHistory[updatedHistory.length - 1];
        root.currentRouteId = prevEntry.id;
        root.currentRouteParams = prevEntry.params;
        return true;
    }

    function popToRoot() {
        if (!root.stackView)
            return;
        while (root.stackView.depth > 1)
            root.stackView.pop(StackView.Immediate);
        if (root.history.length > 0) {
            const rootEntry = root.history[0];
            root.history = [rootEntry];
            root.currentRouteId = rootEntry.id;
            root.currentRouteParams = rootEntry.params;
        }
    }

    onStackViewChanged: {
        // Fired when the StackView binding resolves (may happen after Component.onCompleted
        // because NanRouter is typically declared before StackView in the parent).
        root._doInitialNavigation();
    }

    onRoutesChanged: {
        if (root.currentRouteId === undefined && root.routes.length > 0)
            root.currentRouteId = root.firstRouteId();
    }

    onCurrentRouteIdChanged: {
        if (root.currentRouteId === undefined)
            return;
        if (root.history.length === 0)
            root.history = [
                {
                    id: root.currentRouteId,
                    params: {}
                }
            ];
        root._doInitialNavigation();
    }

    Component.onCompleted: {
        if (root.currentRouteId === undefined)
            root.currentRouteId = root.firstRouteId();
        if (root.currentRouteId !== undefined && root.history.length === 0)
            root.history = [
                {
                    id: root.currentRouteId,
                    params: {}
                }
            ];
        root._doInitialNavigation();
    }
}
