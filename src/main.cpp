
#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>

#include <deque>

using namespace geode::prelude;

namespace pib {
    struct Stage {
        int normal;
        int wave;
    };

    constexpr Stage STAGES[] = {
        {2, 2},
        {4, 4},
        {8, 8},
        {16, 8}
    };

    constexpr int OFF = 4;

    int currentStage() {
        auto raw = Mod::get()->getSettingValue<int64_t>("stage");

        if (raw < 0 || raw > OFF) {
            return 0;
        }

        return static_cast<int>(raw);
    }

    int delayFor(GJBaseGameLayer* layer, bool player1) {
        const int stage = currentStage();

        if (stage == OFF) {
            return 0;
        }

        auto* player = player1
            ? layer->m_player1
            : layer->m_player2;

        const bool wave = player && player->m_isDart;

        return wave
            ? STAGES[stage].wave
            : STAGES[stage].normal;
    }
}

struct PIBPendingInput {
    bool down;
    int button;
    bool player1;
    int remaining;
};

class $modify(PIBBaseGameLayer, GJBaseGameLayer) {
    struct Fields {
        std::deque<PIBPendingInput> queue;
        bool applying = false;
    };

    void handleButton(bool down, int button, bool player1) {
        auto* fields = m_fields.self();
        const int delay = pib::delayFor(this, player1);

        if (fields->applying || delay == 0) {
            GJBaseGameLayer::handleButton(
                down, button, player1
            );
            return;
        }

        fields->queue.push_back({
            down,
            button,
            player1,
            delay
        });

        if (Mod::get()->getSettingValue<bool>("verbose-log")) {
            log::info(
                "[InputBuffer] queued {} for {} ticks",
                down ? "press" : "release",
                delay
            );
        }
    }

    void processCommands(
        float dt,
        bool isHalfTick,
        bool isLastTick
    ) {
        auto* fields = m_fields.self();

        for (auto it = fields->queue.begin();
             it != fields->queue.end();) {

            --it->remaining;

            if (it->remaining <= 0) {
                fields->applying = true;

                GJBaseGameLayer::handleButton(
                    it->down,
                    it->button,
                    it->player1
                );

                fields->applying = false;
                it = fields->queue.erase(it);
            } else {
                ++it;
            }
        }

        GJBaseGameLayer::processCommands(
            dt, isHalfTick, isLastTick
        );
    }
};

class $modify(PIBPlayLayer, PlayLayer) {
    void resetLevel() {
        PlayLayer::resetLevel();
    }
};

$on_mod(Loaded) {
    log::info(
        "[Input Buffer Mobile] Loaded. "
        "Configure the buffer stage in Geode settings."
    );
}
