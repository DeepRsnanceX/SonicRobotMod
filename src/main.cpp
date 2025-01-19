#include <Geode/modify/PlayerObject.hpp>
#include <Geode/loader/SettingV3.hpp>

using namespace geode::prelude;

static int maxFrames = 4;
auto chosenGameSprite = Mod::get()->getSettingValue<std::string>("selected-sprite");
auto isCompatDisabled = Mod::get()->getSettingValue<bool>("disable-compat");
auto isModEnabled = Mod::get()->getSettingValue<bool>("enable-sonicmod");
auto enableSounds = Mod::get()->getSettingValue<bool>("enable-sfx");
auto globalSounds = Mod::get()->getSettingValue<bool>("global-sfx");
auto doIdleAnim = false;

// Individual SFX settings
auto selectedJumpSound = Mod::get()->getSettingValue<std::string>("jump-sfx");
auto selectedOrbSound = Mod::get()->getSettingValue<std::string>("orb-sfx");
auto selectedDashStartSound = Mod::get()->getSettingValue<std::string>("dash-start-sfx");
auto selectedDashStopSound = Mod::get()->getSettingValue<std::string>("dash-stop-sfx");
auto selectedPadSound = Mod::get()->getSettingValue<std::string>("pad-sfx");

$on_mod(Loaded) {
    listenForSettingChanges("selected-sprite", [](std::string value) {
        chosenGameSprite = value;
    });
    listenForSettingChanges("disable-compat", [](bool value) {
        isCompatDisabled = value;
    });
    listenForSettingChanges("enable-sonicmod", [](bool value) {
        isModEnabled = value;
    });
    listenForSettingChanges("enable-sfx", [](bool value) {
        enableSounds = value;
    });
    listenForSettingChanges("global-sfx", [](bool value) {
        globalSounds = value;
    });
    // Individual SFX settings
    listenForSettingChanges("jump-sfx", [](std::string value) {
        selectedJumpSound = value;
    });
    listenForSettingChanges("orb-sfx", [](std::string value) {
        selectedOrbSound = value;
    });
    listenForSettingChanges("dash-start-sfx", [](std::string value) {
        selectedDashStartSound = value;
    });
    listenForSettingChanges("dash-stop-sfx", [](std::string value) {
        selectedDashStopSound = value;
    });
    listenForSettingChanges("pad-sfx", [](std::string value) {
        selectedPadSound = value;
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
        bool m_isShadow = false; // HE GETS SPECIAL TREATMENT BC HES SO FUCKING COOL
        CCSprite* m_customSprite = nullptr;
    };

    bool init(int p0, int p1, GJBaseGameLayer* p2, cocos2d::CCLayer* p3, bool p4) {
        if (!PlayerObject::init(p0, p1, p2, p3, p4)) return false;

        if (isModEnabled){

            auto fields = m_fields.self();

            // Change frames depending on what sprite u selected
            // Some sprites need to use 8 frames max
            // Some need only 4
            if (chosenGameSprite == "mania" || chosenGameSprite == "advance2" || chosenGameSprite == "supermania" || chosenGameSprite == "sonic2hd" || chosenGameSprite == "sonic3maniafied" || chosenGameSprite == "sonic1maniafied" || chosenGameSprite == "classicshadowslide" || chosenGameSprite == "modernsonic") {
                fields->m_maxFrames = 8;
                fields->m_isUsingExtendedFrames = true;
            } else {
                if (chosenGameSprite == "shadow") {
                    fields->m_maxFrames = 12;
                    fields->m_isUsingExtendedFrames = true;
                    fields->m_isShadow = true; // la creatura ... ha llegado
                } else {
                    fields->m_maxFrames = 4;
                    fields->m_isUsingExtendedFrames = false;
                }
            }

            // give birth to sonic (real)
            std::string frameName = fmt::format("{}_sonicRun_01.png"_spr, chosenGameSprite);
            fields->m_customSprite = CCSprite::createWithSpriteFrameName(frameName.c_str());
            if (fields->m_customSprite) {
                fields->m_customSprite->setAnchorPoint({0.5f, 0.5f});
                fields->m_customSprite->setPosition(this->getPosition());
                fields->m_customSprite->setVisible(false);
                fields->m_customSprite->setID("sonic-anim"_spr);
                this->addChild(fields->m_customSprite, 10);
            }

            // Hide robot
            m_robotBatchNode->setVisible(false);

        }

        return true;
    }

    void createRobot(int p0) {
        PlayerObject::createRobot(p0);

        if (isModEnabled){
            m_robotBatchNode->setVisible(false);
            m_robotSprite->setVisible(false);
        }
    }

    void update(float p0) {
        PlayerObject::update(p0);

        if(isModEnabled){

            auto fields = m_fields.self();

            // Sync rotation
            if (fields->m_customSprite && m_mainLayer) {
                fields->m_customSprite->setRotation(m_mainLayer->getRotation());

                // y flip
                bool mainLayerFlippedY = m_mainLayer->getScaleY() < 0;
                fields->m_customSprite->setFlipY(mainLayerFlippedY);
            }

            // Check if ur a robot
            if (!m_isRobot || !fields->m_customSprite) {
                if (fields->m_customSprite) {
                    fields->m_customSprite->setVisible(false);
                }
                return;
            }

            fields->m_customSprite->setVisible(true);

            m_robotFire->setVisible(false);
            m_robotBurstParticles->setVisible(false);

            // bump anim for pads
            if (fields->m_bumpTimer > 0.f) {

                std::string frameName = fmt::format("{}_sonicBumped_01.png"_spr, chosenGameSprite);
                fields->m_customSprite->setDisplayFrame(CCSpriteFrameCache::sharedSpriteFrameCache()->spriteFrameByName(frameName.c_str()));
                fields->m_bumpTimer -= 0.2f;
                fields->m_animationTimer = 0; 
                return;
            }

            // idle anim for platformer
            // the whole catalyst for this mod
            if (m_isPlatformer && m_platformerXVelocity == 0 && m_isOnGround) {

                std::string frameName = fmt::format("{}_sonicIdle_01.png"_spr, chosenGameSprite);
                fields->m_customSprite->setDisplayFrame(CCSpriteFrameCache::sharedSpriteFrameCache()->spriteFrameByName(frameName.c_str()));
                fields->m_animationTimer = 0;
            } else {
                // set current animation (run or jump)
                std::string frameName;
                float frameDuration;

                if (this->m_isOnGround || this->m_hasGroundParticles) {
                    frameName = fmt::format("{}_sonicRun_0{}.png"_spr, chosenGameSprite, fields->m_currentFrame);
                    // change frame times if using extended (8) frames
                    if (fields->m_isUsingExtendedFrames){
                        if (fields->m_isShadow){
                            frameDuration = 2.2f;
                        } else {
                            frameDuration = 1.9f;
                        }
                    } else {
                        frameDuration = 2.2f; // haha 2.2 lol lmao xd
                    }
                } else {
                    frameName = fmt::format("{}_sonicJump_0{}.png"_spr, chosenGameSprite, fields->m_currentFrame);
                    // change frame times if using extended (8) frames
                    if (fields->m_isUsingExtendedFrames){
                        frameDuration = 0.7f;
                    } else {
                        frameDuration = 1.3f;
                    }
                }

                // update animation frame
                fields->m_animationTimer += p0;

                if (fields->m_animationTimer >= frameDuration && fields->m_bumpTimer <= 0.1f) {
                    fields->m_animationTimer -= frameDuration;

                    // update current frame (cycle through 1-4/8)
                    fields->m_currentFrame = (fields->m_currentFrame % fields->m_maxFrames) + 1;
                    auto frame = CCSpriteFrameCache::sharedSpriteFrameCache()->spriteFrameByName(frameName.c_str());

                    // Only update if valid
                    if (frame) {
                        fields->m_customSprite->setDisplayFrame(frame);
                    }
                }
            }

            // if i need to add anything else
            // do it here
            // this is INSIDE the if mod is enabled check

        }

        // this is outside the check

    }

    void doReversePlayer(bool p0) {
        PlayerObject::doReversePlayer(p0);

        if (isModEnabled){
            auto fields = m_fields.self();

            if (p0 != fields->m_flippedX) {
                fields->m_flippedX = p0;
                fields->m_customSprite->setFlipX(p0); 
            }

        }
    }

    void bumpPlayer(float p0, int p1, bool p2, GameObject* p3) {
        PlayerObject::bumpPlayer(p0, p1, p2, p3);

        if (isModEnabled) {
            auto fields = m_fields.self();
            auto fmod = FMODAudioEngine::sharedEngine();
            auto sfxToPlay = fmt::format("{}.ogg"_spr, selectedPadSound);

            if (m_isRobot && fields->m_customSprite) {
                fields->m_bumpTimer = 12.5f; 
            }

            if (enableSounds) {
                if (!globalSounds){
                    if (m_isRobot){
                        fmod->playEffect(sfxToPlay);
                    }
                } else {
                    fmod->playEffect(sfxToPlay);
                }
            }

        }
        
    }

    void playerDestroyed(bool p0) {
        PlayerObject::playerDestroyed(p0);

        auto fields = m_fields.self();
        auto frameName = fmt::format("{}_sonicDeath_01.png"_spr, chosenGameSprite);
        auto deathAnim = CCEaseBackIn::create( CCMoveBy::create(0.75f, {0, -200}) );

        if (isModEnabled && m_isRobot) {
            fields->m_customSprite->setDisplayFrame(CCSpriteFrameCache::sharedSpriteFrameCache()->spriteFrameByName(frameName.c_str()));
            fields->m_customSprite->runAction(deathAnim);
            if (!m_isRobot) {
                fields->m_customSprite->setVisible(false);
            }
        }

    }

    void startDashing(DashRingObject* p0){
        PlayerObject::startDashing(p0);

        auto fmod = FMODAudioEngine::sharedEngine();
        auto sfxToPlayDashStart = fmt::format("{}.ogg"_spr, selectedDashStartSound);

        if (isModEnabled && enableSounds){
            if (!globalSounds){
                if (m_isRobot){
                    fmod->playEffect(sfxToPlayDashStart);
                }
            } else {
                fmod->playEffect(sfxToPlayDashStart);
            }
        }
    }

    // fun fact this is only here
    // bc this runs once every time u start an attempt
    // so i use this to reset the position
    // after the death effect happened
    void stopDashing(){
        PlayerObject::stopDashing();

        if (isModEnabled){
            auto fields = m_fields.self();

            fields->m_customSprite->stopAllActions();
            fields->m_customSprite->setPosition({0,0});

            auto fmod = FMODAudioEngine::sharedEngine();
            auto sfxToPlayDashStop = fmt::format("{}.ogg"_spr, selectedDashStopSound);
            if (enableSounds){
                if (!globalSounds){
                    if (m_isRobot){
                        fmod->playEffect(sfxToPlayDashStop);
                    }
                } else {
                    fmod->playEffect(sfxToPlayDashStop);
                }
            }
        }
    }

    void incrementJumps(){
        PlayerObject::incrementJumps();

        auto fmod = FMODAudioEngine::sharedEngine();
        auto sfxToPlayJump = fmt::format("{}.ogg"_spr, selectedJumpSound);
        auto sfxToPlayOrb = fmt::format("{}.ogg"_spr, selectedOrbSound);

        if (isModEnabled && enableSounds && PlayLayer::get()){
            if (!m_ringJumpRelated){
                if (m_isRobot){
                    fmod->playEffect(sfxToPlayJump);
                }
            } else {
                if (m_ringJumpRelated){
                    if (!globalSounds) {
                        if (m_isRobot){
                            fmod->playEffect(sfxToPlayOrb);
                        }
                    } else {
                        fmod->playEffect(sfxToPlayOrb);
                    }
                }
            }
        }

    }

    void setVisible(bool visible) {
        PlayerObject::setVisible(visible);

        if (isModEnabled){
            auto fields = m_fields.self();

            if (fields->m_customSprite) {
                fields->m_customSprite->setVisible(visible);
            }
        }

    }
};