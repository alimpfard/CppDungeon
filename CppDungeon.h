/*
 * Copyright (c) 2022, Ali Mohammad Pur <mpfard@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include "Basics.h"

template<typename TLeft, typename TForward, typename TRight>
struct MapLevel;

struct SpiderMask {
    template<typename...>
    static constexpr auto describe = s<"a spider mask">;
};

struct LockedChest {
    template<typename...>
    static constexpr auto describe = s<"a locked chest">;
};

struct SmallKey {
    template<typename...>
    static constexpr auto describe = s<"a small key">;
};

struct TokenOfVictory {
    template<typename...>
    static constexpr auto describe = s<"a CD containing full documentation of and a compiler for Herb Sutter's Cpp2">;
};

template<typename... Entries>
struct Items {
    template<auto N>
    using Nth = typename TemplateParameterAtIndexImpl<N, Entries...>::Type;

    static constexpr auto count = sizeof...(Entries);
    using IndexType = decltype(count);
    using Indices = MakeIntegerSequence<IndexType, count>;
};

template<auto V>
struct Health {
    static constexpr auto value = V;
    static_assert(value > 0, "Game Over :(");

    template<bool should_decrement>
    using Decremented = Conditional<should_decrement, Health<V - 1>, Health<V>>;

    template<typename...>
    static constexpr auto describe = concat_strings<to_string<V>, s<" hit points left">>;
};

template<typename... Ts>
struct Inventory {
    template<typename Items>
    using With = decltype([]<auto... Is>(IntegerSequence<typename Items::IndexType, Is...>) {
        return Inventory<typename Items::template Nth<Is>..., Ts...>();
    }(typename Items::Indices()));

    template<typename Item>
    static constexpr auto Has = (IsSame<Item, Ts> || ...);

    template<typename...>
    static constexpr auto describe = [] {
        if constexpr (sizeof...(Ts) == 0)
            return s<"">;
        else
            return concat_strings<s<", You have ">, concat_strings<Ts::template describe<>, s<", ">>..., s<"and some air in your inventory">>;
    }();
};

template<typename Map, auto index>
struct Level {
    using Layout = typename Map::template Level<index>;
};

template<typename Level, typename Layout = typename Level::Layout>
constexpr inline auto MapLevelDescription = concat_strings<
    s<"On your left you notice ">, Layout::Left::description,
    s<"; In front of you there's ">, Layout::Forward::description,
    s<"; On your right you see ">, Layout::Right::description>;

template<typename Level, typename Layout = typename Level::Layout>
constexpr inline auto DescribeRoomIfAvailable = concat_strings<s<"You see ">, Layout::description, s<".">>;

template<typename L, typename F, typename R>
constexpr inline auto DescribeRoomIfAvailable<MapLevel<L, F, R>> = MapLevelDescription<MapLevel<L, F, R>>;

template<typename THealth, typename TInventory, typename TLevel>
struct State {
    using Health = THealth;
    using Inventory = TInventory;
    using Level = TLevel;

    template<typename...>
    static constexpr auto describe = concat_strings<
        DescribeRoomIfAvailable<Level>,
        s<" You have ">, Health::template describe<>, Inventory::template describe<>, s<".">>;

    static constexpr auto won = Health::value > 0 && Inventory::template Has<TokenOfVictory>;

    static consteval void assert_win()
    {
        static_assert(DependentBool<State::won, State::describe<>>, "Win the game to compile :)");
    }
};

template<typename... Levels>
struct GameMapImpl {
    template<auto N>
    using Level = typename TemplateParameterAtIndexImpl<N, Levels...>::Type;
};

template<String Type, String Description, String InsideDescription, typename TActions, typename TTarget>
struct GameEntity {
    static constexpr auto type = Type;
    static constexpr auto description = Description;
    static constexpr auto inside_description = InsideDescription;
    using Actions = TActions;
    using Target = TTarget;
    using Layout = GameEntity<Type, Description, InsideDescription, TActions, TTarget>;
};

enum class KeyRoom {
    key,
    exit,
};

enum class SpidersRoomSouthDoor {
    runForDoor,
    exit,
};

enum class SpidersRoomEastDoor {
    runForDoor,
};

enum class SpidersRoom {
    putOnMask,
};

enum class Chest {
    unlock,
    force,
    leave,
};

template<typename ItemsRequired, auto Outcome, typename ItemsGiven = Items<>>
struct ActionOutcome {
    using items_required = ItemsRequired;
    using items_given = ItemsGiven;
    static constexpr auto outcome = Outcome;
};

template<bool, typename A, auto action, auto scan_index>
struct ActionMappingsTypeImpl;

template<typename A, auto action, auto scan_index>
struct ActionMappingsTypeImpl<true, A, action, scan_index> {
    using Type = typename A::template Nth<scan_index>::outcome;
};

template<typename A, auto action, auto scan_index>
struct ActionMappingsTypeImpl<false, A, action, scan_index> {
    using Type = typename A::template outcome_for_action_impl<action, scan_index + 1>::Type;
};

template<typename... Mappings>
struct ActionMappings {
    template<auto N>
    using Nth = typename TemplateParameterAtIndexImpl<N, Mappings...>::Type;

    static constexpr auto count = sizeof...(Mappings);

    template<String action, auto scan_index>
    struct outcome_for_action_impl {
        using Type = typename ActionMappingsTypeImpl<Nth<scan_index>::input == action, ActionMappings<Mappings...>, action, scan_index>::Type;
    };

    template<String action>
    struct outcome_for_action_impl<action, count> {
        static_assert(DependentFalse<concat_strings<s<"No outcome for action ">, action, s<".">>>);
    };

    template<String action>
    using outcome_for_action = typename outcome_for_action_impl<action, 0>::Type;

    template<typename...>
    static constexpr auto describe = [] {
        if constexpr (sizeof...(Mappings) == 0)
            return s<"">;
        else
            return concat_strings<s<" ">, concat_strings<s<"You can try to '">, Mappings::input, s<"'; ">>...>;
    }();
};

template<auto Input, typename Outcome>
struct ActionMapping {
    static constexpr auto input = Input;
    using outcome = Outcome;
};

template<auto description, auto inside_description, typename Actions>
using MakeRoom = GameEntity<s<"room">, description, inside_description, Actions, void>;

template<typename Item, typename Actions>
using MakeItem = GameEntity<s<"item">, Item::template describe<>, s<"it's an item.">, Actions, Item>;

template<typename TRoom>
struct EnterRoom {
    using Room = TRoom;
};

template<auto description, typename target>
using MakeCorridor = GameEntity<
    s<"corridor">,
    description,
    s<"It's a corridor, there's nothing inside.">,
    ActionMappings<
        ActionMapping<s<"pass through">, ActionOutcome<Items<>, target {}>>>,
    target>;

using MakeWall = GameEntity<s<"wall">, s<"a wall">, s<"It's a wall, there's more wall inside.">, ActionMappings<>, void>;

template<auto I>
struct Target {
    static constexpr auto index = I;
};

template<typename TLeft, typename TForward, typename TRight>
struct MapLevel {
    using Left = TLeft;
    using Forward = TForward;
    using Right = TRight;
    using Actions = ActionMappings<
        ActionMapping<s<"go left">, ActionOutcome<Items<>, EnterRoom<Left> {}>>,
        ActionMapping<s<"go forward">, ActionOutcome<Items<>, EnterRoom<Forward> {}>>,
        ActionMapping<s<"go right">, ActionOutcome<Items<>, EnterRoom<Right> {}>>>;
    using Layout = MapLevel<TLeft, TForward, TRight>;

    static constexpr auto description = DescribeRoomIfAvailable<Layout>;
};

using GameMap = GameMapImpl<
    MapLevel<
        MakeRoom<
            s<"a half-opened door">,
            s<"Opening the door, you find yourself in a room with a small desk. The desk has one drawer.">,
            ActionMappings<
                ActionMapping<s<"Open the drawer">, ActionOutcome<Items<>, KeyRoom::key, Items<SmallKey>>>,
                ActionMapping<s<"Exit the room">, ActionOutcome<Items<>, KeyRoom::exit>>>>,
        MakeCorridor<s<"a dark and humid corridor">, Target<1>>,
        MakeWall>,
    MapLevel<
        MakeWall,
        MakeRoom<
            s<"a door - and you can feel a current of air coming through it">,
            s<"You open the door to a room full of venomous spiders. There's another door on the right wall.">,
            ActionMappings<
                ActionMapping<s<"Reach for the other door">, ActionOutcome<Items<>, SpidersRoomSouthDoor::runForDoor>>,
                ActionMapping<s<"Exit from where you came">, ActionOutcome<Items<>, SpidersRoomSouthDoor::exit>>,
                ActionMapping<s<"Put on the mask">, ActionOutcome<Items<SpiderMask>, SpidersRoom::putOnMask, Items<TokenOfVictory>>>>>,
        MakeCorridor<s<"a tunnel that bifurcates">, Target<2>>>,
    MapLevel<
        MakeCorridor<s<"a the continuation of the tunnel you were in">, Target<3>>,
        MakeWall,
        MakeItem<
            LockedChest,
            ActionMappings<
                ActionMapping<s<"Use the Small Key">, ActionOutcome<Items<SmallKey>, Chest::unlock, Items<SpiderMask>>>,
                ActionMapping<s<"Force it open">, ActionOutcome<Items<>, Chest::force>>,
                ActionMapping<s<"Leave it">, ActionOutcome<Items<>, Chest::leave>>>>>,
    MapLevel<
        MakeCorridor<s<"that following the tunnel is now your only option">, Target<4>>,
        MakeWall,
        MakeWall>,
    MapLevel<
        MakeWall,
        MakeRoom<
            s<"a door - and you can feel an air current coming from below it">,
            s<"You open the door to a completely dark room. As the door slams shut behind you, you notice another door on the left wall...and lots of venomous spiders">,
            ActionMappings<
                ActionMapping<s<"Exit through the other door">, ActionOutcome<Items<>, SpidersRoomEastDoor::runForDoor>>,
                ActionMapping<s<"Put on the mask">, ActionOutcome<Items<SpiderMask>, SpidersRoom::putOnMask, Items<TokenOfVictory>>>>>,
        MakeWall>>;

using NewState = State<
    Health<2>,
    Inventory<>,
    Level<GameMap, 0>>;

template<auto Description, typename TState>
struct GameState {
    static constexpr auto description = concat_strings<
        Description,
        TState::Level::Layout::Actions::template describe<>>;
    using State = TState;

    GameState()
    {
        State::assert_win();
    }
};

using NewGame = GameState<
    concat_strings<
        s<"Welcome Adventurer, You've managed to lock yourself in this type hell, and you can't get out. ">,
        MapLevelDescription<NewState::Level>,
        s<", to play the game, use Act<GameState, Actions...> to perform a series of actions. Good luck!">>,
    NewState>;

template<typename TGameState, typename ActionOutcome, typename RequiredItems, typename ResultingItems, auto TransitionKey>
struct TransitionImpl;

template<typename TGameState, typename ActionOutcome, typename RequiredItems, typename ResultingItems, typename Room, EnterRoom<Room> Key>
struct TransitionImpl<TGameState, ActionOutcome, RequiredItems, ResultingItems, Key> {
    using State = ::State<
        typename TGameState::State::Health,
        typename TGameState::State::Inventory::template With<ResultingItems>,
        Room>;
    static constexpr auto description = Room::inside_description;
};

template<typename TGameState, typename ActionOutcome, typename RequiredItems, typename ResultingItems, auto I, Target<I> target>
struct TransitionImpl<TGameState, ActionOutcome, RequiredItems, ResultingItems, target> {
    using State = ::State<
        typename TGameState::State::Health,
        typename TGameState::State::Inventory::template With<ResultingItems>,
        typename Level<GameMap, I>::Layout>;
    static constexpr auto description = s<"You go through the corridor.">;
};

template<typename TGameState, typename ActionOutcome, typename RequiredItems, typename ResultingItems, KeyRoom key>
struct TransitionImpl<TGameState, ActionOutcome, RequiredItems, ResultingItems, key> {
    using State = ::State<
        typename TGameState::State::Health,
        typename TGameState::State::Inventory::template With<ResultingItems>,
        Level<GameMap, 0>::Layout>;

    static constexpr auto description = [] {
        if constexpr (key == KeyRoom::key)
            return s<"You find a small key! You put it in your pocket and exit the room, leaving the door open, as you found it.">;
        else if constexpr (key == KeyRoom::exit)
            return s<"You leave the room.">;
        else
            compiletime_fail("Inconsistent key");
    }();
};

template<typename TGameState, typename ActionOutcome, typename RequiredItems, typename ResultingItems, Chest key>
struct TransitionImpl<TGameState, ActionOutcome, RequiredItems, ResultingItems, key> {
    using State = ::State<
        typename TGameState::State::Health::template Decremented<key == Chest::force>,
        typename TGameState::State::Inventory::template With<ResultingItems>,
        typename decltype(Switch<key,
            Chest::unlock, TypeWrapper<Level<GameMap, 2>::Layout> {},
            Chest::force, TypeWrapper<typename TGameState::State::Level> {},
            Chest::leave, TypeWrapper<Level<GameMap, 2>::Layout> {},
            0>)::Type>;

    static constexpr auto description = [] {
        if constexpr (key == Chest::unlock)
            return s<"You grab the small key from your pocket; it fits in the chest lock! *click* Inside the chest you find a hairy mask with multiple eyes; You put it in your backpack and close the chest.">;
        else if constexpr (key == Chest::force)
            return s<"As you rattle the chest's lock, a spider comes out of nowhere and bites you.">;
        else if constexpr (key == Chest::leave)
            return s<"You leave the chest alone.">;
        else
            compiletime_fail("Inconsistent key");
    }();
};

template<typename TGameState, typename ActionOutcome, typename RequiredItems, typename ResultingItems, SpidersRoom key>
struct TransitionImpl<TGameState, ActionOutcome, RequiredItems, ResultingItems, key> {
    using State = ::State<
        typename TGameState::State::Health,
        typename TGameState::State::Inventory::template With<ResultingItems>,
        GameEntity<s<"the end">, s<"that you've won the game">, s<"Congrats! Templates await you in the code">, ActionMappings<>, void>>;

    static constexpr auto description = [] {
        return s<"As soon as you put on the mask, the spiders gather around you."
                 " As you move, they spread out, making way for you."
                 " You focus your efforts on investigating the room and find a hidden passage that leads you to..."
                 "The lost treasure of Herb Sutter! You did it, adventurer! You discovered Cpp2!">;
    }();
};

template<typename TGameState, typename ActionOutcome, typename RequiredItems, typename ResultingItems, SpidersRoomSouthDoor key>
struct TransitionImpl<TGameState, ActionOutcome, RequiredItems, ResultingItems, key> {
    using State = ::State<
        typename TGameState::State::Health::template Decremented<key == SpidersRoomSouthDoor::runForDoor>,
        typename TGameState::State::Inventory::template With<ResultingItems>,
        typename decltype(Switch<key,
            SpidersRoomSouthDoor::runForDoor, TypeWrapper<Level<GameMap, 1>::Layout> {},
            SpidersRoomSouthDoor::exit, TypeWrapper<Level<GameMap, 1>::Layout> {},
            0>)::Type>;

    static constexpr auto description = [] {
        if constexpr (key == SpidersRoomSouthDoor::runForDoor)
            return s<"You make a run for the other door, only to find it locked. You go back, but as you leave the room, you are bitten by a rogue spider">;
        else if constexpr (key == SpidersRoomSouthDoor::exit)
            return s<"You go back to where you came from and avoid getting hurt">;
        else
            compiletime_fail("Inconsistent key");
    }();
};

template<typename TGameState, typename ActionOutcome, typename RequiredItems, typename ResultingItems, SpidersRoomEastDoor key>
struct TransitionImpl<TGameState, ActionOutcome, RequiredItems, ResultingItems, key> {
    using State = ::State<
        typename TGameState::State::Health::template Decremented<true>,
        typename TGameState::State::Inventory::template With<ResultingItems>,
        Level<GameMap, 1>::Layout>;

    static constexpr auto description = [] {
        return s<"You make a run for the other door, and as you cross it, you are bitten by a spider">;
    }();
};

template<typename TGameState, typename ActionOutcome>
using Transition = TransitionImpl<TGameState, ActionOutcome, typename ActionOutcome::items_required, typename ActionOutcome::items_given, ActionOutcome::outcome>;

template<typename TGameState, auto TAction>
struct ActImpl {
    using Outcome = Transition<TGameState, typename TGameState::State::Level::Layout::Actions::template outcome_for_action<TAction>>;
    using Result = GameState<
        concat_strings<Outcome::description, s<" ">, Outcome::State::template describe<>>,
        typename Outcome::State>;
};

template<typename TGameState, auto... TActions>
struct ActDetail;

template<typename TGameState, String TAction, auto... TActions>
struct ActDetail<TGameState, TAction, TActions...> {
    using Result = typename ActDetail<typename ActImpl<TGameState, TAction>::Result, TActions...>::Result;
};

template<typename TGameState>
struct ActDetail<TGameState> {
    using Result = TGameState;
};

template<typename TGameState, String... TActions>
using Act = typename ActDetail<TGameState, TActions...>::Result;

template<typename Game>
constexpr inline auto ShowGameState = [] {
    return Game::description;
    // static_assert(DependentFalse<description>);
    // return 0;
}();

template<typename Game>
using GameStateAsType = Type<Game::description>;
