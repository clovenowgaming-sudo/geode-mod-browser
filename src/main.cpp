#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/utils/web.hpp>
#include <Geode/utils/file.hpp>
#include <filesystem>
#include <string>
#include <unordered_map>

using namespace geode::prelude;

class ModBrowserPopup : public Popup {
    CCTextInputNode* m_serverInput = nullptr;
    CCLabelBMFont* m_status = nullptr;
    CCNode* m_list = nullptr;
    async::TaskHolder<web::WebResponse> m_request;
    std::unordered_map<CCObject*, std::pair<std::string, std::string>> m_downloads;

    std::string serverURL() const {
        if (m_serverInput) {
            auto value = m_serverInput->getString();
            if (!value.empty()) return std::string(std::string_view(value));
        }
        return "https://mods.vinzgd.space";
    }

    void setStatus(std::string const& value) {
        if (m_status) m_status->setString(value.c_str());
    }

    static std::string field(matjson::Value const& object, char const* key, std::string fallback = "") {
        auto value = object.get(key);
        if (!value) return fallback;
        return value.unwrap().asString().unwrapOr(fallback);
    }

    void onConnect(CCObject*) {
        auto url = serverURL();
        while (!url.empty() && url.back() == '/') url.pop_back();
        setStatus("Connecting...");
        m_request.spawn(
            web::WebRequest().get(url + "/mods.json"),
            [this](web::WebResponse response) {
                if (!response.ok()) {
                    setStatus("HTTP error: " + std::to_string(response.code()));
                    return;
                }
                auto parsed = response.json();
                if (!parsed || !parsed.unwrap().contains("mods")) {
                    setStatus("Invalid mods.json manifest");
                    return;
                }
                setStatus("Server online - manifest loaded");
                showMods(parsed.unwrap()["mods"]);
            }
        );
    }

    void showMods(matjson::Value const& mods) {
        if (!m_list) return;
        m_list->removeAllChildren();
        float y = 62.f;
        if (!mods.isArray()) {
            setStatus("Manifest 'mods' must be an array");
            return;
        }
        for (auto const& mod : mods) {
            if (!mod.isObject()) continue;
            auto name = field(mod, "name", "Unknown mod");
            auto version = field(mod, "version");
            auto id = field(mod, "id");
            auto gd = field(mod, "gd", "2.2081");
            auto geode = field(mod, "geode", "5.11.2");
            auto download = field(mod, "download");
            auto label = CCLabelBMFont::create((name + "  v" + version).c_str(), "goldFont.fnt");
            label->setAnchorPoint({0.f, .5f});
            label->setScale(.42f);
            label->setPosition({-142.f, y});
            m_list->addChild(label);

            auto meta = CCLabelBMFont::create((id + " | GD " + gd + " | Geode " + geode).c_str(), "chatFont.fnt");
            meta->setAnchorPoint({0.f, .5f});
            meta->setScale(.38f);
            meta->setPosition({-142.f, y - 16.f});
            m_list->addChild(meta);

            if (!download.empty()) {
                auto buttonLabel = CCLabelBMFont::create("Download", "goldFont.fnt");
                buttonLabel->setScale(.42f);
                auto button = CCMenuItemLabel::create(buttonLabel);
                button->setTarget(this, menu_selector(ModBrowserPopup::onDownloadClick));
                m_downloads[button] = {download, gd + "|" + geode};
                auto menu = CCMenu::create();
                menu->setPosition({125.f, y - 5.f});
                menu->addChild(button);
                m_list->addChild(menu);
            }
            y -= 43.f;
            if (y < -115.f) break;
        }
    }

    void onDownloadClick(CCObject* sender) {
        auto it = m_downloads.find(sender);
        if (it == m_downloads.end()) return;
        auto split = it->second.second.find('|');
        auto gd = it->second.second.substr(0, split);
        auto geode = split == std::string::npos ? std::string() : it->second.second.substr(split + 1);
        if (gd != "2.2081" || geode != "5.11.2") {
            FLAlertLayer::create("ModBrowser", "This mod does not target GD 2.2081 / Geode 5.11.2.", "OK")->show();
            return;
        }
        downloadMod(it->second.first);
    }

    void downloadMod(std::string const& url) {
        setStatus("Downloading mod...");
        m_request.spawn(
            web::WebRequest().get(url),
            [this](web::WebResponse response) {
                if (!response.ok()) {
                    setStatus("Download failed: HTTP " + std::to_string(response.code()));
                    return;
                }
                auto path = Mod::get()->getSaveDir() / "downloads" / "modbrowser-download.geode";
                std::error_code ec;
                std::filesystem::create_directories(path.parent_path(), ec);
                if (ec) {
                    setStatus("Could not create download folder");
                    return;
                }
                auto result = utils::file::writeBinary(path, response.data());
                if (!result) {
                    setStatus("Could not save .geode file");
                    return;
                }
                setStatus("Downloaded: " + path.filename().string());
                FLAlertLayer::create("ModBrowser", "Downloaded .geode package to the ModBrowser save folder. Install it using Geode's file installer.", "OK")->show();
            }
        );
    }

    bool init() {
        if (!Popup::init(360.f, 260.f, "GJ_square01.png")) return false;
        this->setTitle("ModBrowser", "bigFont.fnt", 0.8f);

        auto subtitle = CCLabelBMFont::create("Custom mod server", "goldFont.fnt");
        subtitle->setScale(.55f);
        subtitle->setPosition({0.f, 91.f});
        m_mainLayer->addChild(subtitle);

        auto serverLabel = CCLabelBMFont::create("Server URL", "bigFont.fnt");
        serverLabel->setScale(.42f);
        serverLabel->setAnchorPoint({0.f, .5f});
        serverLabel->setPosition({-153.f, 64.f});
        m_mainLayer->addChild(serverLabel);

        m_serverInput = CCTextInputNode::create(260.f, 30.f, "https://mods.vinzgd.space", "chatFont.fnt");
        m_serverInput->setPosition({0.f, 39.f});
        m_serverInput->setString("https://mods.vinzgd.space");
        m_mainLayer->addChild(m_serverInput);

        auto connectLabel = CCLabelBMFont::create("Connect", "goldFont.fnt");
        connectLabel->setScale(.48f);
        auto connectButton = CCMenuItemLabel::create(connectLabel, this, menu_selector(ModBrowserPopup::onConnect));
        auto menu = CCMenu::create();
        menu->setPosition({0.f, 5.f});
        menu->addChild(connectButton);
        m_mainLayer->addChild(menu);

        m_status = CCLabelBMFont::create("Enter a server URL and connect", "chatFont.fnt");
        m_status->setScale(.42f);
        m_status->setPosition({0.f, -21.f});
        m_mainLayer->addChild(m_status);

        m_list = CCNode::create();
        m_list->setPosition({0.f, -30.f});
        m_mainLayer->addChild(m_list);

        auto hint = CCLabelBMFont::create("Manifest: /mods.json | Android 64 | GD 2.2081", "chatFont.fnt");
        hint->setScale(.34f);
        hint->setPosition({0.f, -112.f});
        m_mainLayer->addChild(hint);
        return true;
    }

public:
    static ModBrowserPopup* create() {
        auto ret = new ModBrowserPopup();
        if (ret->init()) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }
};

class $modify(VinzModBrowserMenu, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;
        auto sprite = CCSprite::createWithSpriteFrameName("GJ_downloadBtn_001.png");
        if (!sprite) sprite = CCSprite::createWithSpriteFrameName("GJ_optionsBtn_001.png");
        if (!sprite) return true;
        auto button = CCMenuItemSpriteExtra::create(sprite, this, menu_selector(VinzModBrowserMenu::onOpenModBrowser));
        auto menu = this->getChildByID("bottom-menu");
        if (menu && button) {
            button->setID("vinz-mod-browser-button"_spr);
            menu->addChild(button);
            menu->updateLayout();
        }
        return true;
    }

    void onOpenModBrowser(CCObject*) {
        ModBrowserPopup::create()->show();
    }
};
