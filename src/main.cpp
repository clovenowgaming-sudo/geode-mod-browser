#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/ui/GeodeUI.hpp>

using namespace geode::prelude;

class $modify(VinzModBrowserMenu, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;

        auto sprite = CCSprite::createWithSpriteFrameName("GJ_downloadBtn_001.png");
        if (!sprite) sprite = CCSprite::createWithSpriteFrameName("GJ_optionsBtn_001.png");
        if (!sprite) return true;

        auto button = CCMenuItemSpriteExtra::create(
            sprite,
            this,
            menu_selector(VinzModBrowserMenu::onOpenModBrowser)
        );

        auto menu = this->getChildByID("bottom-menu");
        if (menu && button) {
            button->setID("vinz-mod-browser-button"_spr);
            menu->addChild(button);
            menu->updateLayout();
        }

        return true;
    }

    void onOpenModBrowser(CCObject*) {
        geode::openModsList();
    }
};
