////////////////////////////////////////////////
#include <SFML/Graphics.hpp>
#include <CNtity/Helper.hpp>
#include <random>
#include <iostream>
#include <algorithm>

////////////////////////////////////////////////
const unsigned int ELEMENT_COUNT = 100;
const unsigned int MAX_X = 960;
const unsigned int MAX_Y = 480;
const unsigned int WIDTH = 40;
const unsigned int HEIGHT = 40;

////////////////////////////////////////////////
struct Position
{
    int x;
    int y;

    float distance(const Position& autre)
    {
        return std::sqrt(std::pow(autre.x - x, 2) + std::pow(autre.y - y, 2));
    }
};

////////////////////////////////////////////////
struct Rock {};

////////////////////////////////////////////////
struct Paper {};

////////////////////////////////////////////////
struct Scissors {};

////////////////////////////////////////////////
using Manager = CNtity::Helper<Position, Rock, Paper, Scissors>;

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

    System(Manager& helper) : m_helper(helper) {}
    virtual ~System() {}

    virtual void update() = 0;

    virtual State state() 
    {
        return State::NONE;
    }

protected: 
    Manager& m_helper;
};

////////////////////////////////////////////////
class Collision : public System
{
public:
    Collision(Manager& helper) : System(helper)
    {
        
    }

    virtual System::State state()
    {
        return System::State::NONE;
    }

    template <typename Type, typename Enemy>
    void collide_with_enemy(const Position& position)
    {
        auto&& enemies = m_helper.acquire<Position, Enemy>();

        for(auto&& [e, p]: enemies)
        {
            auto&& pos = std::get<Position>(*p);
            if(sf::FloatRect(pos.x, pos.y, WIDTH, HEIGHT).intersects(sf::FloatRect(position.x, position.y, WIDTH, HEIGHT)))
            {
                m_helper.remove<Enemy>(e);
                m_helper.add<Type>(e, {});
            }
        }
    }

    virtual void update()
    {
        auto positions = m_helper.acquire<Position>();

        for(auto [entity, p]: positions)
        {
            auto&& position = std::get<Position>(*p);

            if(m_helper.has<Rock>(entity))
            {
                collide_with_enemy<Rock, Scissors>(position);
                continue;
            }
                
            if(m_helper.has<Scissors>(entity))
            {
                collide_with_enemy<Scissors, Paper>(position);
                continue;
            }
                
            if(m_helper.has<Paper>(entity))
            {
                collide_with_enemy<Paper, Rock>(position);
                continue;
            }
        }
    }

private:
};


////////////////////////////////////////////////
class Movement : public System
{
public:
    Movement(Manager& helper) : System(helper)
    {
        
    }

    virtual System::State state()
    {
        return System::State::NONE;
    }

    template <typename Type>
    Position closest_of_type(Position& position, bool& found)
    {
        auto elements = m_helper.acquire<Position, Type>();
        auto min = std::min_element(elements.begin(), elements.end(), [&](auto first, auto second)
        {
            return position.distance(std::get<Position>(*first.second)) < position.distance(std::get<Position>(*second.second));
        });

        if(min != elements.end())
            found = true;

        return found ? std::get<Position>(*(min->second)) : Position{};
    }

    virtual void update()
    {
        if(m_clock.getElapsedTime().asMilliseconds() > m_interval)
        {
            m_helper.for_each<Position>([&](auto entity, auto position)
            {
                Position enemy;
                bool found = false;

                if(m_helper.has<Rock>(entity))
                    enemy = closest_of_type<Scissors>(*position, found);
                else if(m_helper.has<Scissors>(entity))
                    enemy = closest_of_type<Paper>(*position, found);
                else if(m_helper.has<Paper>(entity))
                    enemy = closest_of_type<Rock>(*position, found);

                if(found)
                {
                    Position vector = {enemy.x - position->x, enemy.y - position->y};

                    if(vector.x != 0)
                        position->x += vector.x / std::abs(vector.x);

                    if(vector.y != 0)
                        position->y += vector.y / std::abs(vector.y);
                }  
            });

            m_clock.restart();
        }
    }

private:
    sf::Clock m_clock;
    int m_interval = 30;
};

////////////////////////////////////////////////
class Render : public System
{
public:
    Render(Manager& helper) : System(helper), m_window(sf::VideoMode(MAX_X, MAX_Y), "Rock Paper Scissors"), m_paused(false)
    {
        m_ui_view.setCenter(m_window.getSize().x / 2, m_window.getSize().y / 2);
        m_ui_view.setSize(m_window.getSize().x, m_window.getSize().y);

        m_texture_rps.loadFromFile("./assets/rps.png");
    }

    virtual System::State state()
    {
        return m_window.isOpen() ? (m_paused ? System::State::PAUSE : System::State::PLAY) : System::State::CLOSE;
    }
    
    static void dump(Manager& helper)
    {
        helper.for_each<Position>([&](auto entity, auto position) {
            auto components = helper.retrieve(entity);
            std::sort(components.begin(), components.end(), [](auto first, auto second)
            {
                return first->index() < second->index();
            });

            std::cout << entity << " ";
            for(auto&& component: components)
            {
                switch(component->index())
                {
                    case 0: 
                        std::cout << "Position ";
                        break;
                    case 1: 
                        std::cout << "Rock ";
                        break;
                    case 2: 
                        std::cout << "Paper ";
                        break;
                    case 3: 
                        std::cout << "Scissors ";
                        break;
                    default:
                        break;
                }
            }
            std::cout << std::endl;
        });

        int count = 0;
        helper.for_each<Rock>([&](auto entity, auto element) { count++; });
        std::cout << std::dec << "rock count: " << count << std::endl;

        count = 0;
        helper.for_each<Paper>([&](auto entity, auto element) { count++; });
        std::cout << "paper count: " << count << std::endl;

        count = 0;
        helper.for_each<Scissors>([&](auto entity, auto element) { count++; });
        std::cout << "scissors count: " << count << std::endl;

        std::cout << std::endl;
    }

    virtual void update()
    {
        sf::Event event;
        while(m_window.pollEvent(event))
        {
            if(event.type == sf::Event::Closed)
                m_window.close();
            else if(event.type == sf::Event::Resized)
            {
                m_ui_view.setCenter(m_window.getSize().x / 2, m_window.getSize().y / 2);
                m_ui_view.setSize(m_window.getSize().x, m_window.getSize().y);
            }
            else if(event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Space)
            {
                dump(m_helper);
            }
        }

        m_window.clear(sf::Color::White);

        m_window.setView(m_ui_view);

        m_helper.for_each<Position>([&](auto entity, auto position)
        {
            sf::Transform translation;
            translation.translate(position->x, position->y);

            sf::Sprite sprite(m_texture_rps);

            if(m_helper.has<Rock>(entity))
                sprite.setTextureRect({650, 350, 350, 230});
            else if(m_helper.has<Paper>(entity))
                sprite.setTextureRect({400, 670, 420, 310});
            else if(m_helper.has<Scissors>(entity))
                sprite.setTextureRect({300, 140, 270, 450});

            sprite.setScale(WIDTH / sprite.getGlobalBounds().width, HEIGHT / sprite.getGlobalBounds().height);

            m_window.draw(sprite, sf::RenderStates(translation));
        });

        m_window.display();
    }

private:
    sf::RenderWindow m_window;
    sf::View m_ui_view;
    sf::Clock m_clock;
    sf::Texture m_texture_rps;
    bool m_paused;
};

////////////////////////////////////////////////
int main() 
{
    Manager helper;

    std::random_device m_device;  
    std::mt19937 m_gen(m_device()); 
    std::uniform_int_distribution<> m_distrib_type(0, 2);
    std::uniform_int_distribution<> m_distrib_pos_x(0, MAX_X - WIDTH);
    std::uniform_int_distribution<> m_distrib_pos_y(0, MAX_Y - HEIGHT);

    for(unsigned int i = 0; i < ELEMENT_COUNT; ++i)
    {
        int type = m_distrib_type(m_gen);

        auto&& element = helper.create<Position>({m_distrib_pos_x(m_gen), m_distrib_pos_y(m_gen)});

        switch(type)
        {
            case 0:
                helper.add<Rock>(element, {});
                break;
            case 1:
                helper.add<Paper>(element, {});
                break;
            default:
                helper.add<Scissors>(element, {});
                break;
        }
    }

    std::vector<std::shared_ptr<System>> systems 
    { 
        // std::make_shared<Factory>(helper),
        std::make_shared<Movement>(helper),
        std::make_shared<Collision>(helper),
        std::make_shared<Render>(helper)
    };

    bool pause = false;
    while(true)
    {
        for(unsigned int i = 0; i < systems.size(); ++i)
        {
            auto state = systems[i]->state();
            
            if(state == System::State::CLOSE)
                return 0;
            else if(state == System::State::PAUSE || state == System::State::END)
                pause = true;
            else if(state == System::State::PLAY)
                pause = false;

            if(pause && state != System::State::PAUSE)
                continue;

            systems[i]->update();
        }
    }

    return 0;
}