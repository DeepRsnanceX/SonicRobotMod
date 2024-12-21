#include <Geode/Geode.hpp>
#include <Geode/Modify.hpp>
#include <Geode/utils/cocos.hpp>
#include <Geode/loader/SettingV3.hpp>
#include <cocos2d.h>

using namespace geode::prelude;

static int maxFrames = 4;

auto chosenGameSprite = Mod::get()->getSettingValue<std::string>("selected-sprite");

$execute {
    listenForSettingChanges("selected-sprite", [](std::string value) {
        chosenGameSprite = value;
    });
}

class $modify(PlayerObject) {
    struct Fields {
        float m_animationTimer = 0.f;
        float m_bumpTimer = 0.f; 
        int m_maxFrames = 4; 
        int m_currentFrame = 1; 
        bool m_flippedX = false; 
        bool m_flippedY = false; 
        bool m_isUsingExtendedFrames = false;
        CCSprite* m_customSprite = nullptr;
        cocos2d::CCNode* m_mainLayer = nullptr; 
    };

    bool init(int p0, int p1, GJBaseGameLayer* p2, cocos2d::CCLayer* p3, bool p4) {
        if (!PlayerObject::init(p0, p1, p2, p3, p4)) return false;

        // Change frames depending on what sprite u selected
        // Some sprites need to use 8 frames max
        // Some need only 4
        if (chosenGameSprite == "mania_" || chosenGameSprite == "advance2_" || chosenGameSprite == "supermania_" || chosenGameSprite == "sonic2hd_") {
            m_fields->m_maxFrames = 8;
            m_fields->m_isUsingExtendedFrames = true;
        } else {
            m_fields->m_maxFrames = 4;
            m_fields->m_isUsingExtendedFrames = false;
        }

        // Hide robot node
        m_robotBatchNode->setVisible(false);

        // give birth to sonic (real)
        std::string frameName = fmt::format("{}sonicRun_01.png"_spr, chosenGameSprite);
        m_fields->m_customSprite = CCSprite::createWithSpriteFrameName(frameName.c_str());
        if (m_fields->m_customSprite) {
            m_fields->m_customSprite->setAnchorPoint({0.5f, 0.5f});
            m_fields->m_customSprite->setPosition(this->getPosition());
            m_fields->m_customSprite->setVisible(false);
            m_fields->m_customSprite->setID("sonic-anim"_spr);
            this->addChild(m_fields->m_customSprite, 10);
        }

        return true;
    }

    void update(float dt) override {
        PlayerObject::update(dt);

        // Sync rotation
        if (m_fields->m_customSprite && m_mainLayer) {
            m_fields->m_customSprite->setRotation(m_mainLayer->getRotation());

            // y flip
            bool mainLayerFlippedY = m_mainLayer->getScaleY() < 0;
            m_fields->m_customSprite->setFlipY(mainLayerFlippedY);
        }

        // Check if ur a robot
        if (!m_isRobot || !m_fields->m_customSprite) {
            if (m_fields->m_customSprite) {
                m_fields->m_customSprite->setVisible(false);
            }
            return;
        }

        // It looks like shit with the fire and particles
        // so fuck off
        m_robotFire->setVisible(false);
        m_robotBurstParticles->setVisible(false);

        m_fields->m_customSprite->setVisible(true);

        // bump anim for pads
        if (m_fields->m_bumpTimer > 0.f) {

            std::string frameName = fmt::format("{}sonicBumped_01.png"_spr, chosenGameSprite);
            m_fields->m_customSprite->setDisplayFrame(CCSpriteFrameCache::sharedSpriteFrameCache()->spriteFrameByName(frameName.c_str()));
            m_fields->m_bumpTimer -= 0.2f;
            m_fields->m_animationTimer = 0; 
            return;
        }

        // idle anim for platformer
        // this was the whole catalyst for this mod
        // lol
        if (m_isPlatformer && m_platformerXVelocity == 0 && m_isOnGround) {

            std::string frameName = fmt::format("{}sonicIdle_01.png"_spr, chosenGameSprite);
            m_fields->m_customSprite->setDisplayFrame(CCSpriteFrameCache::sharedSpriteFrameCache()->spriteFrameByName(frameName.c_str()));
            m_fields->m_animationTimer = 0;
        } else {
            // set current animation (run or jump)
            std::string frameName;
            float frameDuration;

            if (this->m_isOnGround || this->m_hasGroundParticles) {
                frameName = fmt::format("{}sonicRun_0{}.png"_spr, chosenGameSprite, m_fields->m_currentFrame);
                // change frame times if using extended (8) frames
                if (m_fields->m_isUsingExtendedFrames){
                    frameDuration = 1.9f;
                } else {
                    frameDuration = 2.1f;
                }
            } else {
                frameName = fmt::format("{}sonicJump_0{}.png"_spr, chosenGameSprite, m_fields->m_currentFrame);
                // change frame times if using extended (8) frames
                if (m_fields->m_isUsingExtendedFrames){
                    frameDuration = 0.7f;
                } else {
                    frameDuration = 1.0f;
                }
            }

            // update animation frame
            m_fields->m_animationTimer += dt;

            if (m_fields->m_animationTimer >= frameDuration && m_fields->m_bumpTimer <= 0.1f) {
                m_fields->m_animationTimer -= frameDuration;

                // update current frame (cycle through 1-4/8)
                m_fields->m_currentFrame = (m_fields->m_currentFrame % m_fields->m_maxFrames) + 1;
                auto frame = CCSpriteFrameCache::sharedSpriteFrameCache()->spriteFrameByName(frameName.c_str());

                // Only update if valid
                if (frame) {
                    m_fields->m_customSprite->setDisplayFrame(frame);
                }
            }
        }

        // if i need to add anything else
        // do it here

    }

    void bumpPlayer(float p0, int p1, bool p2, GameObject* p3) {
        PlayerObject::bumpPlayer(p0, p1, p2, p3);

        if (m_isRobot && m_fields->m_customSprite) {
            m_fields->m_bumpTimer = 12.5f; 
        }
        
    }

    // hide if u die
    // this kinda doesn't work
    // so uh
    // yeah ...
    void playerDestroyed(bool p0) {
        PlayerObject::playerDestroyed(p0);

        m_robotBatchNode->setVisible(false);
    }

    virtual void setFlipX(bool p0) override {
        if (p0 != m_fields->m_flippedX) {
            m_fields->m_flippedX = p0;
            m_fields->m_customSprite->setFlipX(p0); 
        }

        PlayerObject::setFlipX(p0);
    }

    void doReversePlayer(bool p0) {
        if (p0 != m_fields->m_flippedX) {
            m_fields->m_flippedX = p0;
            m_fields->m_customSprite->setFlipX(p0); 
        }

        PlayerObject::doReversePlayer(p0);
    }

    void setVisible(bool visible) override {
        PlayerObject::setVisible(visible);
        if (m_fields->m_customSprite) {
            m_fields->m_customSprite->setVisible(visible);
        }
    }

    void onExit() override {
        // cleanup custom sprite
        if (m_fields->m_customSprite) {
            m_fields->m_customSprite->removeFromParent();
            m_fields->m_customSprite = nullptr;
        }
        PlayerObject::onExit();
    }
};
