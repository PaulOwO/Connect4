#include "morpion_server.h"
#include <SFML/Network/TcpSocket.hpp>

#include <iostream>
#include <random>

#include "morpion_packet.h"

namespace morpion
{
    void MorpionServer::ReceivePacket()
    {
        if (selector_.wait(sf::milliseconds(20)))
        {
            for (auto& socket : sockets_)
            {
                if (selector_.isReady(socket))
                {
                    sf::Packet receivedPacket;
                    sf::Socket::Status receivingStatus;
                    do
                    {
                        receivingStatus = socket.receive(receivedPacket);
                    } while (receivingStatus == sf::Socket::Partial);

                    Packet statusPacket;
                    receivedPacket >> statusPacket;
                    switch (static_cast<PacketType>(statusPacket.packetType))
                    {
                    case PacketType::MOVE:
                    {
                        MovePacket movePacket;
                        receivedPacket >> movePacket;
                        ManageMovePacket(movePacket);
                        break;
                    }
                    }
                }
            }
        }
    }

    int MorpionServer::CreateRandomNumber()
	{
        std::default_random_engine generator;
        std::uniform_int_distribution<int> distribution(1, 3);
        int dice_roll = distribution(generator);
        return dice_roll;
	}


    PlayerNumber MorpionServer::CheckWinner() const
    {

        if ((stockedMove == randomNumber) && (currentPlay != randomNumber))
        {
            return 0;
        }
        if ((stockedMove == randomNumber) && (currentPlay == randomNumber))
        {
            return 3;
        }
        if ((stockedMove != randomNumber) && (currentPlay == randomNumber))
        {
            return 1;
        }
    }

    void MorpionServer::ManageMovePacket(const MovePacket& movePacket)
    {
        /* bool create = true;
    	if ( create == true)
        {
            std::array<CaseState, 6> c1{};
            std::array<CaseState, 6> c2{};
            std::array<CaseState, 6> c3{};
            std::array<CaseState, 6> c4{};
            std::array<CaseState, 6> c5{};
            std::array<CaseState, 6> c6{};
            std::array<CaseState, 6> c7{};
            c1.fill(CaseState::Empty);
            c2.fill(CaseState::Empty);
            c3.fill(CaseState::Empty);
            c4.fill(CaseState::Empty);
            c5.fill(CaseState::Empty);
            c6.fill(CaseState::Empty);
            c7.fill(CaseState::Empty);
            create = false;
            //std::array<_ARRAY_, 7> line{ c1, c2, c3, c4, c5, c6, c7 };
        }*/
        std::cout << "Player " << movePacket.move.playerNumber + 1 <<
            " made move " << movePacket.move.position << '\n';
        
        if (phase_ != MorpionPhase::GAME)
            return;

        if(currentMoveIndex_ % 2 != movePacket.move.playerNumber)
        {
            //TODO return to player an error msg
            return;
        }

        if(movePacket.move.position > 3 )
        {
            return;
        }

        if (movePacket.move.position < 1)
        {
	        return;
        }

              //TODO si la colonne est rempli return    

        auto& currentMove = moves_[currentMoveIndex_];
        currentMove.position = movePacket.move.position;
        currentMove.playerNumber = movePacket.move.playerNumber;
        currentMoveIndex_++;

        
    	
        //TODO ici ? la simulation de la grille
    	
        EndType endType = EndType::NONE;
        if(currentMoveIndex_ == 5)
        {
            //TODO end of game
            endType = EndType::STALEMATE;
        }
        //TODO check victory condition
        if ((currentMoveIndex_  == 2 ) ||
           (currentMoveIndex_ == 4) )
        {
            currentPlay = movePacket.move.position;
            PlayerNumber winningPlayer = CheckWinner();
            if ((winningPlayer == 1) || (winningPlayer == 0))
            {
                endType = winningPlayer ? EndType::WIN_P2 : EndType::WIN_P1;
            }
        	else if (winningPlayer == 3)
        	{
                endType = EndType::STALEMATE;
        	}
        }
    	else
    	{
            stockedMove = movePacket.move.position;
    	}
        
       
        
        MovePacket newMovePacket = movePacket;
        newMovePacket.packetType = PacketType::MOVE;

        //sent new move to all players
        for(auto& socket: sockets_)
        {
            sf::Packet sentPacket;
            sentPacket << newMovePacket;
            sf::Socket::Status sentStatus;
            do
            {
                sentStatus = socket.send(sentPacket);
            } while (sentStatus == sf::Socket::Partial);
            
        }
        //send end of game packet
        if (endType != EndType::NONE)
        {
            EndPacket endPacket{};
            endPacket.packetType = PacketType::END;
            endPacket.endType = endType;

            //sent new move to all players
            for (auto& socket : sockets_)
            {
                sf::Packet sentPacket;
                sentPacket << endPacket;
                sf::Socket::Status sentStatus;
                do
                {
                    sentStatus = socket.send(sentPacket);
                } while (sentStatus == sf::Socket::Partial);

            }

            phase_ = MorpionPhase::END;
        }
    }

	
	
        
	


    int MorpionServer::Run()
    {
        if (listener_.listen(serverPortNumber) != sf::Socket::Done)
        {
            std::cerr << "[Error] Server cannot bind port: " << serverPortNumber << '\n';
            return EXIT_FAILURE;
        }
        std::cout << "Server bound to port " << serverPortNumber << '\n';

        while (true)
        {
            switch (phase_)
            {
            case MorpionPhase::CONNECTION:
                ReceivePacket();
                UpdateConnectionPhase();
                break;
            case MorpionPhase::GAME:
                ReceivePacket();
                break;
            case MorpionPhase::END:
                return EXIT_SUCCESS;
            default:;
            }
        }
    }

    void MorpionServer::StartNewGame()
    {
        //Switch to Game state
        phase_ = MorpionPhase::GAME;
        //Send game init packet
        std::cout << "Two players connected!\n";

        std::default_random_engine generator;
        std::uniform_int_distribution<int> distribution(0, 1); 
        int dice_roll = distribution(generator);
        randomNumber = CreateRandomNumber();
        

        for (unsigned char i = 0; i < sockets_.size(); i++)
        {
            GameInitPacket gameInitPacket{};
            gameInitPacket.packetType = PacketType::GAME_INIT;
            gameInitPacket.playerNumber = i != dice_roll;
            sf::Packet sentPacket;
            sentPacket << gameInitPacket;
            sf::Socket::Status sentStatus;
            do
            {
                sentStatus = sockets_[i].send(sentPacket);
            } while (sentStatus == sf::Socket::Partial);
        }
    }

    void MorpionServer::UpdateConnectionPhase()
    {
        // accept a new connection
        const auto nextIndex = GetNextSocket();

        if (nextIndex != -1)
        {
            auto& newSocket = sockets_[nextIndex];
            if (listener_.accept(newSocket) == sf::Socket::Done)
            {
                std::cout << "New connection from " <<
                    newSocket.getRemoteAddress().toString() << ':' << newSocket.
                    getRemotePort() << '\n';
                newSocket.setBlocking(false);
                selector_.add(newSocket);
                if (nextIndex == 1)
                {
                    StartNewGame();
                }
            }
        }
    }

    int MorpionServer::GetNextSocket()
    {
        for (int i = 0; i < maxClientNmb; i++)
        {
            if (sockets_[i].getLocalPort() == 0)
            {
                return i;
            }
        }
        return -1;
    }
}
