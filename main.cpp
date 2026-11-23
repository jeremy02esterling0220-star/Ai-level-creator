// AI Level Creator - mod de Geode para Geometry Dash 2.2074
// Autora: Rosa Melano
//
// Añade un botón "IA" en el editor. Al pulsarlo se abre un menú donde
// describes el nivel y ajustas dificultad y longitud; el generador
// coloca los objetos automáticamente en el editor.
//
// Nota: la "IA" es un generador procedural basado en reglas y palabras
// clave (no una red neuronal ni una conexión a internet).

#include <Geode/Geode.hpp>
#include <Geode/modify/EditorUI.hpp>
#include <random>
#include <algorithm>
#include <cctype>
#include <functional>

using namespace geode::prelude;

// IDs de objetos
namespace ID {
    constexpr int BLOCK = 1;
    constexpr int SPIKE = 8;
    constexpr int YELLOW_PAD = 35;
}

constexpr float GRID = 30.f;
constexpr float GROUND_Y = 15.f; // centro de un bloque apoyado en el suelo

struct GenConfig {
    int length = 40;      // número de patrones
    int difficulty = 2;   // 1..5
    bool spikes = true;
    bool blocks = true;
    bool stairs = true;
    bool pads = true;
    unsigned seed = 1;
};

static std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
    return s;
}

static bool has(std::string const& s, char const* k) {
    return s.find(k) != std::string::npos;
}

// Interpreta la descripción del usuario (español e inglés)
static GenConfig parsePrompt(std::string const& raw, int length, int difficulty) {
    GenConfig c;
    c.length = length;
    c.difficulty = difficulty;
    auto s = toLower(raw);

    if (has(s, "facil") || has(s, "fácil") || has(s, "easy")) c.difficulty = 1;
    if (has(s, "normal") || has(s, "medio") || has(s, "medium")) c.difficulty = 2;
    if (has(s, "dificil") || has(s, "difícil") || has(s, "hard")) c.difficulty = 4;
    if (has(s, "demon") || has(s, "demonio") || has(s, "extremo") || has(s, "extreme")) c.difficulty = 5;

    if (has(s, "largo") || has(s, "long")) c.length = std::max(c.length, 80);
    if (has(s, "corto") || has(s, "short")) c.length = std::min(c.length, 20);

    if (has(s, "sin pico") || has(s, "no spike")) c.spikes = false;
    if (has(s, "sin bloque") || has(s, "no block")) c.blocks = false;
    if (has(s, "sin escalera") || has(s, "no stairs")) c.stairs = false;
    if (has(s, "sin pad") || has(s, "sin salto")) c.pads = false;

    // la semilla sale del texto: misma descripción = mismo nivel
    std::hash<std::string> h;
    c.seed = static_cast<unsigned>(h(s));
    if (s.empty()) c.seed = std::random_device{}();
    return c;
}

static int generateLevel(LevelEditorLayer* lel, GenConfig const& cfg) {
    std::mt19937 rng(cfg.seed);
    auto rnd = [&](int a, int b) { return std::uniform_int_distribution<int>(a, b)(rng); };

    int count = 0;
    auto put = [&](int id, float xUnits, float yUnits) {
        lel->createObject(id, CCPoint{xUnits * GRID, yUnits}, false);
        count++;
    };

    // patrones disponibles según opciones
    enum Pat { SINGLE, DOUBLE, TRIPLE, STAIRS, PAD, BLOCKSPIKE };
    std::vector<Pat> pool;
    if (cfg.spikes) {
        pool.push_back(SINGLE);
        pool.push_back(SINGLE);
        if (cfg.difficulty >= 2) pool.push_back(DOUBLE);
        if (cfg.difficulty >= 4) pool.push_back(TRIPLE);
    }
    if (cfg.stairs && cfg.blocks && cfg.spikes) pool.push_back(STAIRS);
    if (cfg.pads && cfg.spikes && cfg.difficulty >= 2) pool.push_back(PAD);
    if (cfg.blocks && cfg.spikes) pool.push_back(BLOCKSPIKE);
    if (pool.empty()) pool.push_back(SINGLE);

    // distancia mínima entre patrones (en bloques): más difícil = más cerca
    int minGap = 12 - cfg.difficulty * 2; // 10..2
    minGap = std::max(minGap, 4);
    float x = 10.f; // empieza con margen

    for (int i = 0; i < cfg.length; i++) {
        auto p = pool[rnd(0, static_cast<int>(pool.size()) - 1)];
        switch (p) {
            case SINGLE:
                put(ID::SPIKE, x, GROUND_Y);
                x += 1;
                break;
            case DOUBLE:
                put(ID::SPIKE, x, GROUND_Y);
                put(ID::SPIKE, x + 1, GROUND_Y);
                x += 2;
                break;
            case TRIPLE:
                for (int k = 0; k < 3; k++) put(ID::SPIKE, x + k, GROUND_Y);
                x += 3;
                break;
            case STAIRS:
                // escalón de 1 bloque, luego de 2, luego pico al bajar
                put(ID::BLOCK, x, GROUND_Y);
                put(ID::BLOCK, x + 1, GROUND_Y);
                put(ID::BLOCK, x + 1, GROUND_Y + GRID);
                put(ID::SPIKE, x + 4, GROUND_Y);
                x += 5;
                break;
            case PAD:
                put(ID::YELLOW_PAD, x, GROUND_Y - 6.f);
                for (int k = 0; k < 4; k++) put(ID::SPIKE, x + 3 + k, GROUND_Y);
                x += 7;
                break;
            case BLOCKSPIKE:
                put(ID::BLOCK, x, GROUND_Y);
                put(ID::SPIKE, x + 3, GROUND_Y);
                x += 4;
                break;
        }
        x += minGap + rnd(0, 4);
    }
    return count;
}

class AIPopup : public Popup<LevelEditorLayer*> {
protected:
    LevelEditorLayer* m_lel = nullptr;
    TextInput* m_input = nullptr;
    CCLabelBMFont* m_diffLabel = nullptr;
    CCLabelBMFont* m_lenLabel = nullptr;
    int m_difficulty = 2;
    int m_length = 40;

    void refreshLabels() {
        m_diffLabel->setString(fmt::format("Dificultad: {}/5", m_difficulty).c_str());
        m_lenLabel->setString(fmt::format("Longitud: {}", m_length).c_str());
    }

    bool setup(LevelEditorLayer* lel) override {
        m_lel = lel;
        this->setTitle("AI Level Creator");

        auto size = m_mainLayer->getContentSize();

        m_input = TextInput::create(260.f, "Ej: nivel dificil con picos y escaleras");
        m_input->setPosition({size.width / 2, size.height - 55.f});
        m_mainLayer->addChild(m_input);

        m_diffLabel = CCLabelBMFont::create("", "bigFont.fnt");
        m_diffLabel->setScale(.5f);
        m_diffLabel->setPosition({size.width / 2 - 20.f, size.height - 95.f});
        m_mainLayer->addChild(m_diffLabel);

        m_lenLabel = CCLabelBMFont::create("", "bigFont.fnt");
        m_lenLabel->setScale(.5f);
        m_lenLabel->setPosition({size.width / 2 - 20.f, size.height - 130.f});
        m_mainLayer->addChild(m_lenLabel);

        auto menu = CCMenu::create();
        menu->setPosition({0, 0});
        menu->setContentSize(size);
        m_mainLayer->addChild(menu);

        auto mk = [&](char const* txt, CCPoint pos, std::function<void()> fn) {
            auto spr = ButtonSprite::create(txt, 30, true, "bigFont.fnt", "GJ_button_04.png", 30.f, .6f);
            auto btn = CCMenuItemExt::createSpriteExtra(spr, [fn](CCObject*) { fn(); });
            btn->setPosition(pos);
            menu->addChild(btn);
        };

        float rx = size.width / 2 + 90.f;
        mk("-", {rx, size.height - 95.f}, [this] { m_difficulty = std::max(1, m_difficulty - 1); refreshLabels(); });
        mk("+", {rx + 35.f, size.height - 95.f}, [this] { m_difficulty = std::min(5, m_difficulty + 1); refreshLabels(); });
        mk("-", {rx, size.height - 130.f}, [this] { m_length = std::max(10, m_length - 10); refreshLabels(); });
        mk("+", {rx + 35.f, size.height - 130.f}, [this] { m_length = std::min(200, m_length + 10); refreshLabels(); });

        auto genSpr = ButtonSprite::create("Generar nivel");
        auto genBtn = CCMenuItemExt::createSpriteExtra(genSpr, [this](CCObject*) {
            auto cfg = parsePrompt(m_input->getString(), m_length, m_difficulty);
            int n = generateLevel(m_lel, cfg);
            FLAlertLayer::create("Listo", fmt::format("Se colocaron {} objetos.", n), "OK")->show();
        });
        genBtn->setPosition({size.width / 2, 30.f});
        menu->addChild(genBtn);

        refreshLabels();
        return true;
    }

public:
    static AIPopup* create(LevelEditorLayer* lel) {
        auto ret = new AIPopup();
        if (ret->initAnchored(330.f, 220.f, lel)) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }
};

class $modify(AIEditorUI, EditorUI) {
    bool init(LevelEditorLayer* lel) {
        if (!EditorUI::init(lel)) return false;

        auto winSize = CCDirector::get()->getWinSize();
        auto spr = ButtonSprite::create("IA");
        auto btn = CCMenuItemExt::createSpriteExtra(spr, [this](CCObject*) {
            AIPopup::create(m_editorLayer)->show();
        });
        btn->setScale(.8f);

        auto menu = CCMenu::create();
        menu->addChild(btn);
        menu->setPosition({45.f, winSize.height - 45.f});
        this->addChild(menu, 100);
        return true;
    }
};
