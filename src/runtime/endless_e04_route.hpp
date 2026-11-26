#pragma once

template<class Input>
bool playEndlessE04(const Input& input) {
    using Endless054::uniqueGame;
    constexpr int shots[][2]={{-83,-84},{-95,0},{104,30},{81,-98},{52,70},{0,108},{-120,43},
                              {108,-7},{-11,107},{4,87},{39,0},{2,-46},{39,4}};
    input();
    for (const auto& shot:shots) {
        if (Endless054::run.active()!=EndlessRules::E04 || uniqueGame.moving()) return false;
        input({shot[0]/127.0f,shot[1]/127.0f},PSP_CTRL_CROSS,PSP_CTRL_CROSS);
        input();
        for (unsigned frame=0;frame<480 && Endless054::run.active()==EndlessRules::E04 && uniqueGame.moving();++frame) input();
    }
    return progress.endlessKey==1 && Endless054::run.cleared()==2 &&
           Endless054::run.active()!=EndlessRules::E04;
}
