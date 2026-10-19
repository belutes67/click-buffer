#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
using namespace geode::prelude;

namespace {
    struct BufState {
        bool pending = false, realHeld = false, replaying = false;
        bool releaseNext = false, p1 = true;
        int age = 0, flash = 0;
    } g;

    bool on() { return Mod::get()->getSettingValue<bool>("enabled"); }
    int maxSteps() { return (int)Mod::get()->getSettingValue<int64_t>("buffer-frames") * 4; }

    bool isCube(PlayerObject* p) {
        return p && !(p->m_isShip || p->m_isBird || p->m_isBall || p->m_isDart ||
                      p->m_isRobot || p->m_isSpider || p->m_isSwing);
    }

    void updateIndicator() {
        auto pl = PlayLayer::get();
        if (!pl || !pl->m_uiLayer) return;
        auto lbl = typeinfo_cast<CCLabelBMFont*>(pl->m_uiLayer->getChildByID("click-buffer-label"));
        if (!lbl) return;
        bool show = Mod::get()->getSettingValue<bool>("show-indicator");
        if (g.flash > 0) { lbl->setString("GO!"); lbl->setColor({80, 255, 80}); lbl->setVisible(show); }
        else if (g.pending) {
            lbl->setString(fmt::format("BUF {}", maxSteps() - g.age).c_str());
            lbl->setColor({255, 200, 60}); lbl->setVisible(show);
        } else lbl->setVisible(false);
    }
}

class $modify(CBGameLayer, GJBaseGameLayer) {
    void handleButton(bool down, int button, bool p1) {
        if (!on() || g.replaying || button != 1 || !PlayLayer::get()
            || m_levelSettings->m_platformerMode) {
            return GJBaseGameLayer::handleButton(down, button, p1);
        }
        auto pl = p1 ? m_player1 : m_player2;
        if (down) {
            g.realHeld = true;
            if (isCube(pl) && !pl->m_isOnGround) { g.pending = true; g.age = 0; g.p1 = p1; }
            else g.pending = false;
        } else {
            g.realHeld = false;
        }
        GJBaseGameLayer::handleButton(down, button, p1);
    }

    void processCommands(float dt) {
        if (on() && PlayLayer::get()) {
            if (g.flash > 0) g.flash--;
            if (g.releaseNext) {
                g.releaseNext = false;
                if (!g.realHeld) {
                    g.replaying = true;
                    GJBaseGameLayer::handleButton(false, 1, g.p1);
                    g.replaying = false;
                }
            }
            if (g.pending) {
                auto pl = g.p1 ? m_player1 : m_player2;
                if (++g.age > maxSteps() || !isCube(pl)) {
                    g.pending = false;
                } else if (pl->m_isOnGround) {
                    g.pending = false;
                    g.replaying = true;
                    if (g.realHeld) GJBaseGameLayer::handleButton(false, 1, g.p1);
                    GJBaseGameLayer::handleButton(true, 1, g.p1);
                    g.replaying = false;
                    if (!g.realHeld) g.releaseNext = true;
                    g.flash = 12;
                }
            }
            updateIndicator();
        }
        GJBaseGameLayer::processCommands(dt);
    }
};

class $modify(CBPlayLayer, PlayLayer) {
    void setupHasCompleted() {
        PlayLayer::setupHasCompleted();
        g = BufState{};
        auto lbl = CCLabelBMFont::create("BUF", "bigFont.fnt");
        lbl->setID("click-buffer-label");
        lbl->setScale(0.5f);
        lbl->setAnchorPoint({0.f, 1.f});
        auto win = CCDirector::get()->getWinSize();
        lbl->setPosition({8.f, win.height - 8.f});
        lbl->setVisible(false);
        m_uiLayer->addChild(lbl);
    }
    void resetLevel() { PlayLayer::resetLevel(); g = BufState{}; updateIndicator(); }
};