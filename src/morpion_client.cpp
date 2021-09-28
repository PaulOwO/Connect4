#include "morpion_client.h"

#include <array>
#include <imgui.h>
#include <imgui_stdlib.h>
#include <iostream>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Network/Packet.hpp>
#include <SFML/Network/Socket.hpp>

#include "morpion_packet.h"

namespace morpion
{
    sf::Socket::Status MorpionClient::Connect(sf::IpAddress address, unsigned short portNumber)
    {
        const auto status = socket_.connect(address, portNumber);
        socket_.setBlocking(false);
        return status;
    }

    MorpionPhase MorpionClient::GetPhase() const
    {
        return phase_;
    }

    bool MorpionClient::IsConnected() const
    {
        return socket_.getLocalPort() != 0;
    }

    void MorpionClient::Init()
    {
    }

    void MorpionClient::ReceivePacket(sf::Packet& packet)
    {
        Packet morpionPacket{};
        packet >> morpionPacket;

        switch (static_cast<PacketType>(morpionPacket.packetType))
        {
        case PacketType::GAME_INIT:
        {
            GameInitPacket gameInitPacket{};
            packet >> gameInitPacket;
            playerNumber_ = gameInitPacket.playerNumber;
            phase_ = MorpionPhase::GAME;
            std::cout << "You are player " << gameInitPacket.playerNumber + 1 << '\n';
            break;
        }
        case PacketType::MOVE:
        {
            if (phase_ != MorpionPhase::GAME)
                break;
            MovePacket movePacket;
            packet >> movePacket;
            std::cout << "Receive move packet from player " <<
                movePacket.move.playerNumber + 1 << " with position: " << movePacket.move.position << '\n';
            auto& currentMove = moves_[currentMoveIndex_];
            currentMove.position = movePacket.move.position;
            currentMove.playerNumber = movePacket.move.playerNumber;
            currentMoveIndex_++;
            break;
        }
        case PacketType::END:
        {
            if (phase_ != MorpionPhase::GAME)
            {
                break;
            }
            EndPacket endPacket;
            packet >> endPacket;
            switch (endPacket.endType)
            {
            case EndType::STALEMATE:
                endMessage_ = "Stalemate";
                break;
            case EndType::WIN_P1:
                endMessage_ = playerNumber_ == 0 ? "You won" : "You lost";
                break;
            case EndType::WIN_P2:
                endMessage_ = playerNumber_ == 1 ? "You won" : "You lost";
                break;
            case EndType::ERROR:
                endMessage_ = "Error";
                break;
            default:;
            }
            phase_ = MorpionPhase::END;
            break;
        }
        default:
            break;
        }
    }

    void MorpionClient::Update()
    {
        //Receive packetS
        if (socket_.getLocalPort() != 0)
        {
            sf::Packet receivedPacket;
            sf::Socket::Status status;
            do
            {
                status = socket_.receive(receivedPacket);
            } while (status == sf::Socket::Partial);

            if (status == sf::Socket::Done)
            {
                ReceivePacket(receivedPacket);
            }

            if (status == sf::Socket::Disconnected)
            {
                socket_.disconnect();
                std::cerr << "Server disconnected\n";
            }
        }
    }

    void MorpionClient::Destroy()
    {
    }

    int MorpionClient::GetPlayerNumber() const
    {
        return playerNumber_;
    }

    void MorpionClient::SendNewMove(int position)
    {
        MovePacket movePacket;
        movePacket.packetType = PacketType::MOVE;
        movePacket.move.position = position;
        movePacket.move.playerNumber = playerNumber_;
        sf::Packet packet;
        packet << movePacket;
        sf::Socket::Status sentStatus;
        do
        {
            sentStatus = socket_.send(packet);
        } while (sentStatus == sf::Socket::Partial);
    }

    unsigned char MorpionClient::GetMoveIndex() const
    {
        return currentMoveIndex_;
    }

    std::string_view MorpionClient::GetEndMessage() const
    {
        return endMessage_;
    }

    MorpionView::MorpionView(MorpionClient& client) : client_(client)
    {
    }

    void MorpionView::DrawImGui()
    {
        switch (client_.GetPhase())
        {
        case MorpionPhase::CONNECTION:
        {
            if (client_.IsConnected())
                return;
            ImGui::Begin("Client");

            ImGui::InputText("Ip Address", &ipAddressBuffer_);
            ImGui::InputInt("Port Number", &portNumber_);
            if (ImGui::Button("Connect"))
            {
                const auto status = client_.Connect(sf::IpAddress(ipAddressBuffer_), portNumber_);
                if (status != sf::Socket::Done)
                {
                    switch (status)
                    {
                    case sf::Socket::NotReady:
                        std::cerr << "Not ready to connect to " << ipAddressBuffer_ << ':' << portNumber_ << '\n';
                        break;
                    case sf::Socket::Partial:
                        std::cerr << "Connecting to " << ipAddressBuffer_ << ':' << portNumber_ << '\n';
                        break;
                    case sf::Socket::Disconnected:
                        std::cerr << "Disconnecting to " << ipAddressBuffer_ << ':' << portNumber_ << '\n';
                        break;
                    case sf::Socket::Error:
                        std::cerr << "Error connecting to " << ipAddressBuffer_ << ':' << portNumber_ << '\n';
                        break;
                    default:;
                    }
                }
                else
                {
                    std::cout << "Successfully connected to server\n";
                }

            }
            ImGui::End();
            break;		
        }
        case MorpionPhase::GAME:
        {
            ImGui::Begin("Game");
            const auto playerNumber = client_.GetPlayerNumber();
            ImGui::InputInt("New Move", &currentColumn);
            if (client_.GetMoveIndex() % 2 == playerNumber)
            {
                if (ImGui::Button("Send"))
                {
                    client_.SendNewMove(currentColumn);
                }
            }
            ImGui::End();
            break;
        }
        case MorpionPhase::END:
        {
            ImGui::Begin("End");
            ImGui::Text("%s", client_.GetEndMessage().data());
            ImGui::End();
            break;
        }
        default:;
        }
    }
}
