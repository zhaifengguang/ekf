// -*- coding:utf-8; mode:c++; mode:auto-fill; fill-column:80; -*-

///
/// @file    GravityAction.cpp
/// @brief   Computes state accelerations and partials due to the
///          interaction of an agent with a gravational body.
/// @author  Jonathon Smith <jonathon.j.smith@gmail.com>
/// @date    January 24, 2015
///

// C++ Standard Library
#include <iostream>
#include <cmath>

// ekf Library
#include <GravityAction.hpp>

//=====================================================================
//=====================================================================
// CONSTRUCTORS / DESCTRUCTOR

// Default Constructor
GravityAction::
GravityAction()
    : m_name(),
      m_radius(),
      m_mu(),
      m_J2(),
      m_evaledPartials()
{
}

// Constructor for standard solar system central body
GravityAction::
GravityAction(
    const std::string name,
    const double radius,
    const double mu,
    const double J2 )
    : m_name( name ),
      m_radius( radius ),
      m_mu( mu ),
      m_J2( J2 ),
      m_evaledPartials()
{
}

// Destructor
GravityAction::
~GravityAction()
{
}

//=====================================================================
//=====================================================================
// PUBLIC MEMBERS

// Computes the acceleration due to central body gravitation and the J2
// perturbation.
void
GravityAction::
getAcceleration(
    std::vector< double > &acceleration,
    const std::vector< double > &state ) const
{
  double dist = sqrt( pow( state[0], 2 ) + pow( state[1], 2 ) +
                pow( state[2], 2 ) );
  acceleration[0] += -m_mu * state[0] / pow( dist, 3 ) * accJ2( state, 'x' );
  acceleration[1] += -m_mu * state[1] / pow( dist, 3 ) * accJ2( state, 'y' );
  acceleration[2] += -m_mu * state[2] / pow( dist, 3 ) * accJ2( state, 'z' );
}

// Computes the partial derivative of the acceleration terms and owned
// parameters
void
GravityAction::
getPartials(
    std::vector< double > &partials,
    const std::vector< double > &state,
    const std::vector< std::string >  &activeAgents )
{
  // Evaluate the class partial for this state
  evalPartials( state );

  // Loop over active agents and get partial values
  int numAgents = activeAgents.size();
  for ( int i = 0; i < numAgents; ++i )
  {
    // Request the partial from the i loop with respect to all the active
    // agents ( j loop )
    for ( int j = 0; j < numAgents; ++j )
    {
      if (m_debug)
      {
        std::cout << "\nGravityAction::getPartials()" << std::endl
                  << "Requested Partials: " << activeAgents[i] <<  " wrt "
                  << activeAgents[j] << std::endl
                  << "Value of partials: "
                  << getAgentPartial( activeAgents[i], activeAgents[j] );
      }
      partials[ i * numAgents + j ] += getAgentPartial( activeAgents[i],
                                                        activeAgents[j] );
    }
  }
}

//=====================================================================
//=====================================================================
// PRIVATE MEMBERS

// Computes the J2 gravitational perturbation, by state component.
double
GravityAction::
accJ2(
    const std::vector< double > &state,
    const char component ) const
{
  double dist = sqrt( pow( state[0], 2 ) + pow( state[1], 2 ) +
                      pow( state[2], 2 ) );

  // This function augments the two-body EOMs with a J2 term.
  if ( ( component == 'x' ) || ( component == 'y' ) )
  {
    return ( 1.0 - 1.5 * m_J2 * pow( ( m_radius / dist ), 2.0 ) *
           ( 5 * pow( ( state[2] / dist ), 2) - 1 ) );
  }
  else if ( component == 'z' )
  {
    return ( 1.0 - 1.5 * m_J2 * pow( ( m_radius / dist ), 2 ) *
           ( 5 * pow( ( state[2] / dist ), 2 ) - 3 ) );
  }
  else
  {
     std::cout << "Error in request for J2 perturbation." << std::endl
               << "Component " << component << " not recognized" << std::endl;
     throw;
  }
}

double
GravityAction::
getAgentPartial(
    const std::string &top,
    const std::string &bottom )
{
  // Form param search string
  std::string partialRequest = top + " wrt " + bottom;

  if( m_evaledPartials.find( partialRequest ) == m_evaledPartials.end() )
  {
    // If requested partial is not supported by this action, return 0
    return 0.0;
  }
  return m_evaledPartials[ partialRequest ];
}

void
GravityAction::
evalPartials( const std::vector< double > &state )
{
  // Condense variable names to make following equations more legible
  double r = sqrt( pow( state[0], 2 ) + pow( state[1], 2 ) +
                   pow( state[2], 2 ) );
  double R = m_radius;
  double mu = m_mu;
  double J2 = m_J2;
  double X = state[0];
  double Y = state[1];
  double Z = state[2];
  double r3 = pow( r, 3 );
  double r5 = pow( r, 5 );
  double R_r2 = pow( R / r, 2 );
  double Z_r2 = pow( Z / r, 2 );

  // Partials of acceleration X component wrt state.
  m_evaledPartials[ "dX wrt X" ] = (
    - mu / r3 * ( 1 - ( 3 / 2 ) * J2 * R_r2 * ( 5 * Z_r2 - 1.) ) +
    3 * mu * pow( X, 2 ) / r5 * ( 1 - ( 5 / 2 ) * J2 * R_r2 *
    ( 7 * Z_r2 - 1 ) ) );
  m_evaledPartials[ "dX wrt Y" ] =
    3 * mu * X * Y / r5 * ( 1 - ( 5 / 2 ) * J2 * R_r2 * ( 7 * Z_r2 - 1 ) );
  m_evaledPartials[ "dX wrt Z" ] =
    3 * mu * X * Z / r5 * ( 1 - ( 5 / 2 ) * J2 * R_r2 * ( 7 * Z_r2 - 3 ) );

  // Partials of acceleration Y component wrt state.
  m_evaledPartials[ "dY wrt X" ] =
    3 * mu * X * Y / r5 * ( 1 - ( 5  / 2 ) * J2 * R_r2 * ( 7 * Z_r2 - 1 ) );
  m_evaledPartials[ "dY wrt Y" ] =
    ( - mu / r3 * ( 1 - ( 3 / 2 ) * J2 * R_r2 * ( 5 * Z_r2 - 1 ) ) +
    3 * mu * pow( Y, 2 ) / r5 * ( 1 - ( 5 / 2 ) * J2 * R_r2 *
    ( 7 * Z_r2 - 1 ) ) );
  m_evaledPartials[ "dY wrt Z" ] =
    3 * mu * Y * Z / r5 * ( 1 - ( 5 / 2 ) * J2 * R_r2 * ( 7 * Z_r2 - 3 ) );

  // Partials of acceleration Z component wrt state.
  m_evaledPartials[ "dZ wrt X" ] =
    3 * mu * X * Z / r5 * ( 1 - ( 5 / 2 ) * J2 * R_r2 * ( 7 * Z_r2 - 3 ) );
  m_evaledPartials[ "dZ wrt Y" ] =
    3 * mu * Y * Z / r5 * ( 1 - ( 5 / 2 ) * J2 * R_r2 * ( 7 * Z_r2 - 3 ) );
  m_evaledPartials[ "dZ wrt Z" ] =
    ( - mu / r3 * ( 1 - ( 3 / 2 ) * J2 * R_r2 * ( 5 * Z_r2 - 3 ) ) +
    3 * mu * pow( Z, 2 ) / r5 * ( 1 - ( 5 / 2 ) * J2 * R_r2 *
    ( 7 * Z_r2 - 5 ) ) );

  /// @todo implement remaining partials:
  ///   - Cartesian state X, Y, Z, dX, dY, dZ components
  ///   - Gravitational body radius
  ///   - Gravitational body GM
  ///   - Gravitational body J2 term
}

