// -*- coding:utf-8; mode:c++; mode:auto-fill; fill-column:80; -*-

///
/// @file    OdeintHelper.hpp
/// @brief   Interface class between ekf and boost::odeint
/// @author  Jonathon Smith <jonathon.j.smith@gmail.com>
/// @date    January 24, 2015
///

// C++ Standard Library
#include <iostream>

// Eigien Library
#include <eigen/dense>

// ekf Library
#include <OdeintHelper.hpp>

//=====================================================================
//=====================================================================
// CONSTRUCTORS / DESCTRUCTOR

OdeintHelper::
OdeintHelper()
    : m_actions(),
      m_activeAgents()
{
}

OdeintHelper::
OdeintHelper(
    std::vector< std::shared_ptr< Action > >& actions,
    std::vector< std::string >& activeAgents )
    : m_actions( &actions ),
      m_activeAgents( &activeAgents )
{
}

OdeintHelper::
~OdeintHelper()
{
}

//=====================================================================
//=====================================================================
// PUBLIC MEMBERS

// This method defines the equations of motion for the odeint
// integrator.
void
OdeintHelper::
operator() (
    const std::vector< double > &x ,
    std::vector< double > &dxdt ,
    const double t  )
{
  // Accumulate accelerations from the different actions.
  std::vector< double > accel( 3, 0.0 );
  for ( auto ap: *m_actions )
  {
    ap->getAcceleration( accel, x );
  }

  // Accumulate partials from the different actions.
  int numAgents = m_activeAgents->size();
  int numPartials = numAgents * numAgents;
  std::vector< double > partials( numPartials, 0.0 );
  for ( auto ap: *m_actions )
  {
    ap->getPartials( partials, x, *m_activeAgents );
  }

  // Write the paramter partials into a matrix
  Eigen::MatrixXd A( numAgents, numAgents );
  A = Eigen::MatrixXd::Zero( numAgents, numAgents );
  for ( int i = 0; i < numAgents ; ++i )
  {
    for ( int j = 0; j < numAgents; ++j )
    {
      A(i, j) = partials[ j + i * numAgents ];
    }
  }

  if ( m_debug )
  {
    std::cout << "\n### A at time " << t << std::endl;
    for ( int i = 0; i < numAgents; ++i )
    {
      for ( int j = 0; j < numAgents; ++j )
      {
        std::cout << "   " << A( i, j );
      }
      std::cout << std::endl;
    }
  }

  // Write the current STM into a matrix
  Eigen::MatrixXd stm( numAgents, numAgents );
  for ( int i = 0; i < numAgents ; ++i )
  {
    for ( int j = 0; j < numAgents; ++j )
    {
      stm(i, j) = x[ 6 + j + i * numAgents ];
    }
  }

  if ( m_debug )
  {
    std::cout << "\n### STM at time " << t << std::endl;
    for ( int i = 0; i < numAgents; ++i )
    {
      for ( int j = 0; j < numAgents; ++j )
      {
        std::cout << "   " << stm( i, j );
      }
      std::cout << std::endl;
    }
  }

  // Multiply the current STM times A partials to get derivative of STM
  Eigen::MatrixXd dStm = A * stm;

  if ( m_debug )
  {
    std::cout << "\n### Derivative of STM at time " << t << std::endl;
    for ( int i = 0; i < numAgents; ++i )
    {
      for ( int j = 0; j < numAgents; ++j )
      {
        std::cout << "   " << dStm( i, j );
      }
      std::cout << std::endl;
    }
  }

  // State elements
  dxdt[0] = x[3]; // X_dot
  dxdt[1] = x[4]; // Y_dot
  dxdt[2] = x[5]; // Z_dot
  dxdt[3] = accel[0]; // DX_dot
  dxdt[4] = accel[1]; // DY_dot
  dxdt[5] = accel[2]; // DY_dot

  // State partials
  for ( int i = 0; i < numAgents; ++i )
  {
    for (int j = 0; j < numAgents; ++j )
    dxdt[ 6 + j + i * numAgents ] = dStm(i,j);
  }
}

/// @todo remove this
void
OdeintHelper::
howManyActions()
{
  std::cout << "There are " << m_actions->size()
       << " Actions in the helper" << std::endl;
}
