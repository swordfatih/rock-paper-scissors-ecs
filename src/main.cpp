////////////////////////////////////////////////
#include <SFML/Graphics.hpp>
#include <CNtity/Helper.hpp>
#include <random>
#include <iostream>
#include <algorithm>

////////////////////////////////////////////////
const unsigned int ELEMENT_COUNT = 200;
const unsigned int MAX_X = 960;
const unsigned int MAX_Y = 480;
const unsigned int WIDTH = 40;
const unsigned int HEIGHT = 40;

////////////////////////////////////////////////
struct Cooldown
{
    sf::Clock clock;
    sf::Time max = sf::milliseconds(500);

    bool expired() const
    {
        return clock.getElapsedTime() > max;
    }
};

////////////////////////////////////////////////
struct Position
{
    int x;
    int y;

    float distance(const Position &autre)
    {
        return std::sqrt(std::pow(autre.x - x, 2) + std::pow(autre.y - y, 2));
    }
};

////////////////////////////////////////////////
struct Rock
{
};

////////////////////////////////////////////////
struct Paper
{
};

////////////////////////////////////////////////
struct Scissors
{
};

////////////////////////////////////////////////
class System
{
public:
    enum class State
    {
        PLAY,
        PAUSE,
        END,
        CLOSE,
        NONE
    };

    System(CNtity::Helper &helper) : m_helper(helper) {}
    virtual ~System() {}

    virtual void update() = 0;

    virtual State state()
    {
        return State::NONE;
    }

protected:
    CNtity::Helper &m_helper;
};

////////////////////////////////////////////////
class Collision : public System
{
public:
    Collision(CNtity::Helper &helper) : System(helper), m_rocks(helper.view<Position, Rock, Cooldown>()), m_papers(helper.view<Position, Paper, Cooldown>()), m_scissors(helper.view<Position, Scissors, Cooldown>())
    {
    }

    virtual System::State state()
    {
        return System::State::NONE;
    }

    template <typename Type, typename Enemy>
    void collision(const Position &position, const CNtity::Entity &entity, const Position &enemy, Cooldown& cooldown_first, Cooldown& cooldown_second)
    {
        if (sf::FloatRect(enemy.x, enemy.y, WIDTH, HEIGHT).intersects(sf::FloatRect(position.x, position.y, WIDTH, HEIGHT)) && cooldown_first.expired() && cooldown_second.expired())
        {
            // m_helper.erase(entity);
            m_helper.remove<Enemy>(entity);
            m_helper.add<Type>(entity, {});
            cooldown_first.clock.restart();
            cooldown_second.clock.restart();
        }
    }

    virtual void update()
    {
        for (auto [entity, position, type, cooldown_first] : m_rocks.each())
        {
            for (auto [e, pos, type, cooldown_second] : m_scissors.each())
            {
                collision<Rock, Scissors>(position, e, pos, cooldown_first, cooldown_second);
            }
        }

        for (auto [entity, position, type, cooldown_first] : m_papers.each())
        {
            for (auto [e, pos, type, cooldown_second] : m_rocks.each())
            {
                if (cooldown_first.clock.getElapsedTime() > cooldown_first.max && cooldown_second.clock.getElapsedTime() > cooldown_second.max)
                {
                    collision<Paper, Rock>(position, e, pos, cooldown_first, cooldown_second);
                }
            }
        }

        for (auto [entity, position, type, cooldown_first] : m_scissors.each())
        {
            for (auto [e, pos, type, cooldown_second] : m_papers.each())
            {
                if (cooldown_first.clock.getElapsedTime() > cooldown_first.max && cooldown_second.clock.getElapsedTime() > cooldown_second.max)
                {
                    collision<Scissors, Paper>(position, e, pos, cooldown_first, cooldown_second);
                }
            }
        }
    }

private:
    CNtity::View<Position, Rock, Cooldown> m_rocks;
    CNtity::View<Position, Paper, Cooldown> m_papers;
    CNtity::View<Position, Scissors, Cooldown> m_scissors;
};

////////////////////////////////////////////////
class Movement : public System
{
public:
    Movement(CNtity::Helper &helper) : System(helper), m_positions(helper.view<Position>())
    {
    }

    virtual System::State state()
    {
        return System::State::NONE;
    }

    template <typename Type>
    Position closest_of_type(Position &position, bool &found)
    {
        auto elements = m_helper.view<Position, Type>().each();
        auto min = std::min_element(elements.begin(), elements.end(), [&](auto first, auto second)
                                    {
            auto [first_ent, first_pos, first_type] = first;
            auto [second_ent, second_pos, second_type] = second;
            return position.distance(first_pos) < position.distance(second_pos); });

        if (min == elements.end())
        {
            found = false;
            return Position{};
        }

        found = true;
        auto [min_ent, min_pos, min_type] = *min;

        return min_pos;
    }

    virtual void update()
    {
        if (m_clock.getElapsedTime().asMilliseconds() > m_interval)
        {
            m_positions.each([&](auto entity, auto &position)
                             {
                Position enemy;
                bool found = false;

                if(m_helper.has<Rock>(entity))
                    enemy = closest_of_type<Scissors>(position, found);
                else if(m_helper.has<Scissors>(entity))
                    enemy = closest_of_type<Paper>(position, found);
                else if(m_helper.has<Paper>(entity))
                    enemy = closest_of_type<Rock>(position, found);

                if(found)
                {
                    Position vector = {enemy.x - position.x, enemy.y - position.y};

                    if(vector.x != 0)
                        position.x += vector.x / std::abs(vector.x);

                    if(vector.y != 0)
                        position.y += vector.y / std::abs(vector.y);
                } 
                
                if(m_helper.has<Rock>(entity))
                    enemy = closest_of_type<Paper>(position, found);
                else if(m_helper.has<Scissors>(entity))
                    enemy = closest_of_type<Rock>(position, found);
                else if(m_helper.has<Paper>(entity))
                    enemy = closest_of_type<Scissors>(position, found);

                if(found)
                {
                    Position vector = {enemy.x - position.x, enemy.y - position.y};

                    if(vector.x != 0)
                        position.x -= vector.x / std::abs(vector.x);

                    if(vector.y != 0)
                        position.y -= vector.y / std::abs(vector.y);
                } 

                if(position.x < 0)
                {
                    position.x = 0;
                }

                if(position.y < 0)
                {
                    position.y = 0;
                }

                if(position.x > MAX_X - WIDTH)
                {
                    position.x = MAX_X - WIDTH;
                }
                
                if(position.y > MAX_Y - HEIGHT)
                {
                    position.y = MAX_Y - HEIGHT;
                } });

            m_clock.restart();
        }
    }

private:
    CNtity::View<Position> m_positions;
    sf::Clock m_clock;
    int m_interval = 15;
};

////////////////////////////////////////////////
class Render : public System
{
public:
    Render(CNtity::Helper &helper) : System(helper), m_positions(m_helper.view<Position>()), m_window(sf::VideoMode(MAX_X, MAX_Y), "Rock Paper Scissors"), m_paused(false)
    {
        m_ui_view.setCenter(m_window.getSize().x / 2, m_window.getSize().y / 2);
        m_ui_view.setSize(m_window.getSize().x, m_window.getSize().y);

        m_texture_rps.loadFromFile("./assets/rps.png");

        initialize();
    }

    virtual System::State state()
    {
        return m_window.isOpen() ? (m_paused ? System::State::PAUSE : System::State::PLAY) : System::State::CLOSE;
    }

    void dump()
    {
        std::cout << std::dec << "rock count: " << m_helper.entities<Rock>().size() << std::endl;
        std::cout << std::dec << "paper count: " << m_helper.entities<Paper>().size() << std::endl;
        std::cout << std::dec << "scissors count: " << m_helper.entities<Scissors>().size() << std::endl;
        std::cout << std::endl;
    }

    void initialize()
    {
        for(auto entity: m_helper.entities())
        {
            m_helper.erase(entity);
        }

        std::random_device m_device;
        std::mt19937 m_gen(m_device());
        std::uniform_int_distribution<> m_distrib_type(0, 2);
        std::uniform_int_distribution<> m_distrib_pos_x(0, MAX_X - WIDTH);
        std::uniform_int_distribution<> m_distrib_pos_y(0, MAX_Y - HEIGHT);

        for (unsigned int i = 0; i < ELEMENT_COUNT; ++i)
        {
            int type = m_distrib_type(m_gen);

            auto &&element = m_helper.create<Position, Cooldown>({m_distrib_pos_x(m_gen), m_distrib_pos_y(m_gen)}, {});

            switch (type)
            {
            case 0:
                m_helper.add<Rock>(element, {});
                break;
            case 1:
                m_helper.add<Paper>(element, {});
                break;
            default:
                m_helper.add<Scissors>(element, {});
                break;
            }
        }
    }

    virtual void update()
    {
        sf::Event event;
        while (m_window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                m_window.close();
            else if (event.type == sf::Event::Resized)
            {
                m_ui_view.setCenter(m_window.getSize().x / 2, m_window.getSize().y / 2);
                m_ui_view.setSize(m_window.getSize().x, m_window.getSize().y);
            }
            else if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Space)
            {
                dump();
            }
            else if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Return)
            {
                initialize();
            }
        }

        m_window.clear(sf::Color::White);

        m_window.setView(m_ui_view);

        m_positions.each([&](auto entity, auto &position)
                         {
            sf::Transform translation;
            translation.translate(position.x, position.y);

            sf::Sprite sprite(m_texture_rps);

            if(m_helper.has<Rock>(entity))
                sprite.setTextureRect({650, 350, 350, 230});
            else if(m_helper.has<Paper>(entity))
                sprite.setTextureRect({400, 670, 420, 310});
            else if(m_helper.has<Scissors>(entity))
                sprite.setTextureRect({300, 140, 270, 450});

            sprite.setScale(WIDTH / sprite.getGlobalBounds().width, HEIGHT / sprite.getGlobalBounds().height);

            m_window.draw(sprite, sf::RenderStates(translation)); });

        m_window.display();
    }

private:
    CNtity::View<Position> m_positions;
    sf::RenderWindow m_window;
    sf::View m_ui_view;
    sf::Clock m_clock;
    sf::Texture m_texture_rps;
    bool m_paused;
};

////////////////////////////////////////////////
int main()
{
    CNtity::Helper helper;

    std::vector<std::shared_ptr<System>> systems{
        // std::make_shared<Factory>(helper),
        std::make_shared<Movement>(helper),
        std::make_shared<Collision>(helper),
        std::make_shared<Render>(helper)};

    bool pause = false;
    while (true)
    {
        for (unsigned int i = 0; i < systems.size(); ++i)
        {
            auto state = systems[i]->state();

            if (state == System::State::CLOSE)
                return 0;
            else if (state == System::State::PAUSE || state == System::State::END)
                pause = true;
            else if (state == System::State::PLAY)
                pause = false;

            if (pause && state != System::State::PAUSE)
                continue;

            systems[i]->update();
        }
    }

    return 0;
}