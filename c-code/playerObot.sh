#!/bin/bash

BROKER="34.145.113.189"

echo "Player O bot starting..."
last_status=""
game_over=false

while true; do
    # Get current status
    status=$(mosquitto_sub -h "$BROKER" -t ttt/game/status -C 1)

    # Game over check
    if [[ "$status" == *"Winner:"* || "$status" == "Draw" ]]; then
        if [ "$game_over" = false ]; then
            echo "Game over: $status"
            echo "Waiting for new game to start..."
            game_over=true
        fi
        sleep 2
        continue
    fi

    # Reset flag once a new game starts
    if [[ "$status" == "Next: X" || "$status" == "Next: O" ]]; then
        if [ "$game_over" = true ]; then
            echo "New game detected."
        fi
        game_over=false
    fi

    # Avoid repeated status messages
    if [[ "$status" != "$last_status" && "$game_over" = false ]]; then
        echo "Status: $status"
        last_status="$status"
    fi

    # Only act if it's Player O's turn
    if [[ "$status" == "Next: O" ]]; then
        # Get board state
        board=$(mosquitto_sub -h "$BROKER" -t ttt/game/state -C 1)

        # Find empty cells
        empty_cells=()
        for i in {0..8}; do
            char="${board:$i:1}"
            if [[ "$char" == " " ]]; then
                empty_cells+=($i)
            fi
        done

        if [[ ${#empty_cells[@]} -eq 0 ]]; then
            sleep 1
            continue
        fi

        # Pick a random move
        rand_index=$((RANDOM % ${#empty_cells[@]}))
        move=${empty_cells[$rand_index]}

        echo "Player O plays at cell $((move + 1))"
        mosquitto_pub -h "$BROKER" -t ttt/game/move/o -m "$move"
        sleep 1
    else
        sleep 1
    fi
done
