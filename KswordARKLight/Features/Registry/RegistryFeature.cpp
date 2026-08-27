#include "RegistryFeature.h"

#include "RegistryView.h"

namespace Ksword::Features::Registry {

HWND CreateRegistryFeaturePage(HWND parent, const RECT& bounds) {
    return CreateRegistryView(parent, bounds);
}

bool RequestRegistryFeatureNavigate(HWND page, const std::wstring& path) {
    return RequestRegistryViewNavigate(page, path);
}

} // namespace Ksword::Features::Registry
