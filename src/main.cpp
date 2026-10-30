#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/ui/GeodeUI.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/utils/web.hpp>
#include <Geode/utils/file.hpp>
#include <Geode/utils/general.hpp>
#include <string>

using namespace geode::prelude;

// ModBrowser uses a small JSON manifest hosted by the server URL entered below.
// Manifest format: {"mods":[{"id":"author.mod","name":"Mod name","version":"1.0","description":"...","gd":"2.2081","geode":"5.11.2","platform":"android64","download":"https://host/file.geode"}]}

class ModBrowserPopup : public Popup {
    CCTextInputNode* m_serverInput = nullptr;
    CCLabelBMFont* m_status = nullptr;
    CCNode* m_list = nullptr;

    std::string serverURL() const {
        if (m_serverInput && m_serverInput->getString() && m_serverInput->getString()[0]) {
            return m_serverInput->getString();
        }
        return "https://mods.vinzgd.space";
    }

    void setStatus(std::string const& value) {
        if (m_status) m_status->setString(value.c_str());
    }

    void onConnect(CCObject*) {
        auto url = serverURL();
        while (!url.empty() && url.back() == '/') url.pop_back();
        setStatus("Connecting...");
        auto req = web::WebRequest();
        req.get(url + "/mods.json").listen([this](web::WebResponse* response) {
            if (!response) {
                setStatus("Server connection failed");
                return;
            }
            if (response->code() < 200 || response->code() >= 300) {
                setStatus("HTTP error: " + std::to_string(response->code()));
                return;
            }
            auto parsed = matjson::parse(response->string());
            if (!parsed || !parsed.unwrap().contains("mods")) {
                setStatus("Invalid mods.json manifest");
                return;
            }
            setStatus("Server online - manifest loaded");
            showMods(parsed.unwrap()["mods"]);
        });
    }

    void showMods(matjson::Value const& mods) {
        if (!m_list) return;
        m_list->removeAllChildren();
        float y = 62.f;
        for (auto const& mod : mods) {
            if (!mod.isObject()) continue;
            auto name = mod.get("name").asString().unwrapOr("Unknown mod");
            auto version = mod.get("version").asString().unwrapOr("");
            auto id = mod.get("id").asString().unwrapOr("");
            auto gd = mod.get("gd").asString().unwrapOr("2.2081");
            auto geode = mod.get("geode").asString().unwrapOr("5.11.2");
            auto download = mod.get("download").asString().unwrapOr("");
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
                auto button = CCMenuItemLabel::create(buttonLabel, [this, download, gd, geode](CCObject*) {
                    if (gd != "2.2081" || geode != "5.11.2") {
                        FLAlertLayer::create("ModBrowser", "This mod does not target GD 2.2081 / Geode 5.11.2.", "OK")->show();
                        return;
                    }
                    downloadMod(download);
                });
                auto menu = CCMenu::create();
                menu->setPosition({125.f, y - 5.f});
                menu->addChild(button);
                m_list->addChild(menu);
            }
            y -= 43.f;
            if (y < -115.f) break;
        }
    }

    void downloadMod(std::string const& url) {
        setStatus("Downloading mod...");
        auto req = web::WebRequest();
        req.get(url).listen([this](web::WebResponse* response) {
            if (!response || response->code() < 200 || response->code() >= 300) {
                setStatus("Download failed");
                return;
            }
            auto path = Mod::get()->getSaveDir() / "downloads" / "modbrowser-download.geode";
            (void) utils::file::createDirectoryAll(path.parent_path());
            auto result = utils::file::writeBinary(path, response->data());
            if (!result) {
                setStatus("Could not save .geode file");
                return;
            }
            setStatus("Saved: " + path.filename().string());
            FLAlertLayer::create("ModBrowser", "Downloaded .geode package to the ModBrowser save folder. Install it using Geode's file installer.", "OK")->show();
        });
    }

    bool init() {
        if (!Popup::init(360.f, 260.f, "GJ_square01.png")) return false;
        this->setTitle("ModBrowser", "bigFont.fnt", 0.8f);

        auto subtitle = CCLabelBMFont::create("Custom mod server", "goldFont.fnt");
        subtitle->setScale(.55f);
        subtitle->setPosition({0.f, 91.f});
        m_mainLayer->addChild(subtitle);

        auto serverLabel = CCLabelBMFont::create("Server / DNS URL", "bigFont.fnt");
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
