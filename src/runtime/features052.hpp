#pragma once

bool check(bool success,const char* message) {
    if (!success) std::fprintf(stderr,"Fixture failed: %s\n",message);
    return success;
}

bool near(Vec2 a,Vec2 b) { return length(a-b)<0.001f; }

void parkGhosts(State& s) {
    for (unsigned i=0;i<s.ghosts.size();++i) s.ghosts[i].position={float(i*80),-5000};
}

bool seaFeatures(const Fixture& f) {
    auto noSwim=f.initial(false);
    f.step(noSwim);
    if (!check(noSwim.loss==Loss::Water,"water kills without swim")) return false;
    auto swimmer=f.initial();
    f.step(swimmer);
    if (!check(!swimmer.terminal(),"authored whole-field water allows swimmer")) return false;
    swimmer.game.position={409,0};
    f.step(swimmer);
    if (!check(swimmer.loss==Loss::Backrooms,"grounded bounds enter Backrooms")) return false;

    auto fake=f.initial();
    parkGhosts(fake);
    fake.game.position=CosmicFakeGoals614[12];
    fake.game.velocity={20,0};
    f.step(fake);
    if (!check(!fake.game.won && fake.fakeGoals.triggered[12]==1 && fake.fakeGoals.bonus==500 &&
               !fake.game.moving(),"decoy stops player and grants score")) return false;
    f.step(fake);
    if (!check(fake.fakeGoals.bonus==500,"decoy scores once")) return false;

    auto nuke=f.initial();
    parkGhosts(nuke);
    while (nuke.tick<300) f.step(nuke);
    if (!check(nuke.chuckles.count==1 && nuke.chuckles.bodies[0].radius>0,"first Chuckle spawns at tick 300"))
        return false;
    nuke.game.position=CosmicNukes614[2];
    f.step(nuke);
    if (!check(nuke.nuke.has(2) && nuke.chuckles.count==0,"Nuke removes live Chuckles")) return false;
    while (nuke.tick<600) f.step(nuke);
    if (!check(nuke.chuckles.count==1 && nuke.chuckles.bodies[0].radius>0,
               "Nuke leaves spawn countdown intact")) return false;
    std::puts("P-14 water eligibility, decoy score and stop, Backrooms, Nuke and tick 600 respawn verified.");
    return true;
}

bool santaFeatures(const Fixture& f) {
    if (!check(near(SantaRun052::goal(0),{0,-144}) && near(SantaRun052::goal(84),{0,-144}) &&
               near(SantaRun052::goal(132),{0,-1047}) && near(SantaRun052::goal(180),{0,-1950}) &&
               near(SantaRun052::goal(360),{0,-1950}),"authored goal abduction keyframes")) return false;
    auto camera=f.initial();
    parkGhosts(camera);
    camera.game.position={-100,-100};
    f.step(camera);
    if (!check(camera.santa.cameraY==-10 && !camera.santa.boundsArmed,
               "camera lerps 0.1 toward highest player point")) return false;
    camera.game.position={-100,-50};
    f.step(camera);
    if (!check(camera.santa.highestY==-100 && camera.santa.cameraY==-19,"camera keeps scrolling up"))
        return false;
    camera.game.position={-257,-300};
    f.step(camera);
    if (!check(camera.santa.boundsArmed && camera.loss==Loss::OutOfBounds,"left exit arms O.B.")) return false;
    f.stuck.setOffset({0,-500});
    const auto stuck=f.stuck.sweep({0,-294},{0,30},f.course.playerRadius);
    if (!check(stuck.time<1 && stuck.normal.y<0,"camera-relative bottom boundary moves with viewport"))
        return false;

    auto traffic=f.initial();
    parkGhosts(traffic);
    for (unsigned i=0;i<61;++i) f.step(traffic);
    if (!check(traffic.cars.liveCount[2]&&traffic.cars.liveCount[3],"ignoreY retains northern car lanes"))
        return false;

    SantaRun052 ring;
    if (!check(!ring.keyActive(0) && ring.startRing({64,-1240},f.course.playerRadius,1) &&
               ring.keyActive(240) && !ring.keyActive(241) && ring.finishRing(false,241) &&
               !ring.ringPassed && !ring.startRing({64,-1240},f.course.playerRadius,242),
               "4-second key timeout and non-repeatable ring")) return false;
    SantaRun052 pass;
    pass.startRing({64,-1240},f.course.playerRadius,1);
    if (!check(pass.finishRing(true,40) && pass.ringPassed && !pass.keyActive(41),"key pickup ends ring"))
        return false;

    for (const auto& pad:CosmicJump615) {
        auto jump=f.initial();
        jump.game.position=pad.center;
        if (!check(jump.jump.tryLaunch(CosmicJump615,std::size(CosmicJump615),jump.game,f.course) &&
                   near(jump.game.position,pad.center) &&
                   near(jump.game.velocity,Vec2{std::cos(pad.radians),std::sin(pad.radians)}*(f.course.stopSpeed+10)),
                   "authored jump pad launches at specified center and angle")) return false;
    }
    std::puts("P-15 abduction timing, camera and boundary tiles, O.B., car despawn, ring timeout/pass and all 30 jump pads verified.");
    return true;
}
