#!/bin/bash

BROKER="34.145.113.189"

echo "Player 2 (Bash) ready..."

# Wait for START command
mosquitto_sub -h $BROKER -t ttt/game/auto -C 1 >/dev/null

while true; do
    status=$(mosquitto_sub -h $BROKER -t ttt/game/status -C 1)

    if [[ "$status" == "Winner:"* || "$status" == "Draw" ]]; then
        sleep 2
        continue
    fi

    if [[ "$status" == "Next: O" ]]; then
        state=$(mosquitto_sub -h $BROKER -t ttt/game/state -C 1)
        for i in {0..8}; do
            if [[ "${state:$i:1}" == " " ]]; then
                empty+=($i)
            fi
        done

        if [[ ${#empty[@]} -gt 0 ]]; then
            move=${empty[$RANDOM % ${#empty[@]}]}
            echo "Bash: Sending move $((move + 1))"
            mosquitto_pub -h $BROKER -t ttt/game/move/o -m "$move"
        fi

        empty=()
        sleep 1
    else
        sleep 1
    fi
done
