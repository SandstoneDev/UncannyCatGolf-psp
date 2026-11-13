// Round methods shared by the gameplay loop and the focused emulator run.
    void beginGoldenTee518() {
        finale518.beginTee(game.position);
        game.won=false;
        canSwim=true;
        controls={};
    }

    void playHorseEvents518(const HorseEvents039& event) {
        if (event.chargeUp) audio.play(Sound::HorseChargeUp);
        if (event.charge) audio.play(Sound::HorseCharge);
        if (event.bounce) audio.play(Sound::HorseBounce);
    }

    void stepFinale518(bool fast) {
        using Phase=FinalePhase518;
        if (caught || finale518.canEnterEnding()) return;
        if (finale518.phase==Phase::Hunt) {
            ++ticks;
            const auto event=finale518.stepHunt(game,scene,horseWalls039,horse,jump,ticks);
            playHorseEvents518(event.horse);
            if (event.sand && !inSand) thoughts.play(Thought::Sand,tvTick());
            inSand=event.sand;
            if (event.launch) {
                controls={};
                audio.play(Sound::JumpPad);
                thoughts.play(Thought::Jump,tvTick());
            }
            if (event.mineHit) {
                constexpr Sound hurts[]={Sound::HorseHurt1,Sound::HorseHurt2,Sound::HorseHurt3};
                audio.play(hurts[sceKernelGetSystemTimeLow()%std::size(hurts)]);
                audio.play(Sound::HorseImpact);
                thoughts.play(Thought::Horse,tvTick());
                audio.musicTune(1,2.0f-horse.health/7.0f);
                if (finale518.phase==Phase::Defeat) {
                    controls={};
                    audio.play(Sound::HorseDeathSupreme);
                }
            }
            if (event.loss!=HuntLoss518::None) {
                caught=true;
                backrooms=event.loss==HuntLoss518::Backrooms;
                fellOut=event.loss==HuntLoss518::OutOfBounds;
                waterDead=event.loss==HuntLoss518::Water;
                finaleMineLoss=event.loss==HuntLoss518::Mine;
                controls={};
                game.velocity={};
                horse.lose();
                if (waterDead) {
                    splashTick=totalTicks;
                    audio.play(Sound::WaterSplash);
                    audio.play(Sound::WaterDeath);
                }
                if (finaleMineLoss) {
                    mineLossTick518=totalTicks;
                    audio.play(Sound::WallDeath);
                }
            }
            return;
        }
        if (finale518.phase==Phase::Defeat) {
            ++horse.age;
            horse.zoomPulse+=(1-horse.zoomPulse)*0.2f;
            horse.shake*=0.9f;
            const unsigned age=finale518.defeatTicks+1;
            if (age<=60) audio.musicTune(1,2.0f*(60-age)/60);
            if ((age<=120 || (age>=300 && age<=480)) && age%15==0) {
                horse.defeatBurst();
                audio.play(Sound::HorseFlightBurst);
            }
            if (age==150 || age==240) audio.playTuned(Sound::ResultsBang,3,0.5f);
            if (age>=300 && age<=462 && (age-300)%9==0)
                audio.playTuned(Sound::HorseMissileBoom,-6,1);
            if (age==540) audio.play(Sound::HorseSurprise);
            if (finale518.advanceDefeat()) {
                horse.phase=HorsePhase039::Finished;
                game.won=true;
                jump={};
                jumpStage={};
            }
            return;
        }
        if (finale518.keyWaiting) {
            ++finale518.keyTicks;
            if (finale518.keyTicks==2402) audio.playTuned(Sound::HorseKeyHoly,0,0.7f);
            if (finale518.keyTicks==3324) audio.play(Sound::HorseKeyProverb);
            if (finale518.keyTicks==3656) audio.playTuned(Sound::HorseKeyProverbDelay,-10,3);
            if (finale518.keyTicks>=HorseFinale518Run::KeyAwardTick) {
                finale518.keyWaiting=false;
                progress.elysianKey=1;
                questKeyTick=totalTicks;
                audio.play(Sound::SecretKey);
            }
        }
        if (finale518.cameraEntered && finale518.cameraTicks<HorseFinale518Run::CameraTicks)
            ++finale518.cameraTicks;
        if (finale518.phase==Phase::Results) return;
        if (finale518.phase==Phase::Drop) {
            if (!finale518.advanceDrop()) return;
            audio.play(Sound::Bounce);
        }
        if (finale518.phase==Phase::TeeReady) {
            if (!finale518.cameraEntered &&
                HorseFinale518Run::overlaps(game.position,scene.playerRadius,ElysianCameraZone518)) {
                finale518.cameraEntered=true;
                finale518.keyWaiting=false;
                audio.stop(Sound::HorseKeyHoly);
                audio.stop(Sound::HorseKeyProverb);
                audio.stop(Sound::HorseKeyProverbDelay);
            }
            const auto* walls=finale518.cameraEntered?ElysianEndWalls518:nullptr;
            const unsigned count=walls?std::size(ElysianEndWalls518):0;
            game.step(walls,count,{},nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,nullptr,nullptr,
                      nullptr,1,{},false,sweepElysianRocks518);
            const bool swimming=touchesElysianWater518(game.position,scene.playerRadius);
            if (swimming && !pool.swimming) {
                audio.play(Sound::WaterSplash);
                thoughts.play(Thought::Swim,tvTick());
            }
            pool.swimming=swimming;
            const Vec2 delta=game.position-ElysianTee518.center;
            const float reach=scene.playerRadius+ElysianTee518.radius;
            if (dot(delta,delta)<=reach*reach) {
                finale518.beginAttract(game.position,game.velocity);
                game.velocity={};
                controls={};
                audio.musicTune(1,1);
                audio.music(Sound::GoldenTeeMusic);
                thoughts.play(Thought::Win,tvTick());
            }
            return;
        }
        if (finale518.phase==Phase::Attract) {
            const unsigned previous=finale518.attractTicks;
            audio.musicTune(fast?0.5f:1.0f,fast?4.0f:1.0f);
            finale518.advanceAttract(fast);
            if (previous<1116 && finale518.attractTicks>=1116) audio.play(Sound::GoldenClubThwack);
        }
    }

    void playMoonCow601() {
        const unsigned entropy=sceKernelGetSystemTimeLow();
        unsigned next=entropy%51==50?5:entropy%5;
        if (next==moonMoo) next=(next+1+entropy%3)%5;
        moonMoo=next;
        const auto sound=next==5?Sound::CarHorn1:
            static_cast<Sound>(static_cast<unsigned>(Sound::MoonMoo1)+next);
        audio.playTuned(sound,5,0.833333f+(entropy%1000)*(1.2f-0.833333f)/999);
    }

    void stepCosmic601() {
        ++ticks;
        if (jump.outside(game.position,scene)) {
            caught=backrooms=true;
            game.velocity={};
            controls={};
            return;
        }
        const unsigned previous=game.bounces;
        game.step(nullptr,0,{},nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,nullptr,nullptr,
                  nullptr,1,{},false,cosmicGateEligible?nullptr:sweepPrismGate601);
        const SolidBlock trigger{PrismGate601.center,PrismGate601.halfSize+Vec2{1,1}};
        const bool inside=HorseFinale518Run::overlaps(game.position,scene.playerRadius,trigger);
        if (inside && !cosmicGateInside && !cosmicGateBroken) {
            if (cosmicGateEligible) {
                cosmicGateBroken=ticks;
                audio.play(Sound::PrismGateShatter);
            } else {
                cosmicGatePoor=ticks;
                audio.play(Sound::SlotSad);
            }
        }
        cosmicGateInside=inside;
        if (game.bounces!=previous && walls.active() &&
            (!inside || cosmicGateEligible) && !HorseFinale518Run::overlaps(game.position,scene.playerRadius,trigger))
            audio.playTuned(Sound::GlassClink,0,0.8f+(sceKernelGetSystemTimeLow()%301)/1000.0f);
        if (!game.won) chaseGhosts(true);
    }
