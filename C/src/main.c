#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <stdio.h>
#include <time.h>
#include "window.h"
#include "shaders.h"
#include "solver.h"

#define max(a, b)   ((a > b) ?  a: b)
#define min(a, b)   ((a < b) ?  a: b)


GLuint shaderProgram;
int main(int argc, char* argv[]) {

    // Initialise window & shaders
    GLFWwindow *window = init_window();
    init_shaders();

    // Create Vertex Array Object
    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // Create a Vertex Buffer Object for positions
    GLuint vbo_pos;
    glGenBuffers(1, &vbo_pos);

    GLfloat *positions = (GLfloat*) malloc(2*N_DISCR*N_DISCR*sizeof(GLfloat));
    for (int i = 0; i < N_DISCR; i++) {
        for (int j = 0; j < N_DISCR; j++) {
            int ind = i*N_DISCR+j;
            positions[2*ind  ] = (float)(-1.0 + 2.0*i/(N_DISCR-1));
            positions[2*ind+1] = (float)(-1.0 + 2.0*j/(N_DISCR-1));
        }
    }

    glBindBuffer(GL_ARRAY_BUFFER, vbo_pos);
    glBufferData(GL_ARRAY_BUFFER, 2*N_DISCR*N_DISCR*sizeof(GLfloat), positions, GL_STATIC_DRAW);

    // Specify vbo_pos' layout
    GLint posAttrib = glGetAttribLocation(shaderProgram, "position");
    glEnableVertexAttribArray(posAttrib);
    glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

    // Create an Element Buffer Object and copy the element data to it
    GLuint ebo;
    glGenBuffers(1, &ebo);

    GLuint *elements = (GLuint*) malloc(4*(N_DISCR-1)*(N_DISCR-1)*sizeof(GLuint));
    for (int i = 0; i < N_DISCR-1; i++) {
        for (int j = 0; j < N_DISCR-1; j++) {
            int ind  = i*N_DISCR+j;
            int ind_ = i*(N_DISCR-1)+j;

            elements[4*ind_  ] = ind;
            elements[4*ind_+1] = ind+1;
            elements[4*ind_+2] = ind+N_DISCR;
            elements[4*ind_+3] = ind+N_DISCR+1;
        }
    }

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 4*(N_DISCR-1)*(N_DISCR-1)*sizeof(GLuint), elements, GL_STATIC_DRAW);

    // Simulation parameters
    int t = 0;
    int skip = 10;
    double dt = TIMESTEP;

    // Initialise Cahn-Hilliard solver
    double *c = (double*) malloc(N_DISCR*N_DISCR*sizeof(double));
    for (int i = 0; i < N_DISCR*N_DISCR; i++) {
        c[i] = .2*((double)rand() / (double)RAND_MAX ) - .1;
    }
    init_solver(c, dt);

    // Create a Vertex Buffer Object for colors
    GLuint vbo_colors;
    glGenBuffers(1, &vbo_colors);

    GLfloat *colors = (GLfloat*) malloc(N_DISCR*N_DISCR*sizeof(GLfloat));
    for (int i = 0; i < N_DISCR; i++) {
        for (int j = 0; j < N_DISCR; j++) {
            int ind = i*N_DISCR+j;
            colors[ind] = (float) ((c[ind] + 1.0)/2.0);
        }
    }

    glBindBuffer(GL_ARRAY_BUFFER, vbo_colors);
    glBufferData(GL_ARRAY_BUFFER, N_DISCR*N_DISCR*sizeof(GLfloat), colors, GL_STREAM_DRAW);

    // Specify vbo_color's layout
    GLint colAttrib = glGetAttribLocation(shaderProgram, "color");
    glEnableVertexAttribArray(colAttrib);
    glVertexAttribPointer(colAttrib, 1, GL_FLOAT, GL_FALSE, 0, (void*)0);

    // Window loop
    double cpu_time;
    clock_t begin, end;
    while (!glfwWindowShouldClose(window)) {
        // Timestepping
        begin = clock();
        for (int i = 0; i < skip; i++) {
            step(dt);
            t++;
        }
        getSolution(c);
        end = clock();

        // Event input
        glfwPollEvents();
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, GL_TRUE);
        }

        // Update graphics
        // begin = clock();
        glfwSwapBuffers(window);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Update plot
        double c_max =  1.0;
        double c_min = -1.0;
        // for (int i = 0; i < N_DISCR*N_DISCR; i++) {
        //     c_max = max(c_max, c[i]);
        //     c_min = min(c_min, c[i]);
        // }
        for (int i = 0; i < N_DISCR*N_DISCR; i++) {
            colors[i] = (float) ((c[i] - c_min) / (c_max - c_min));
        }
        glBindBuffer(GL_ARRAY_BUFFER, vbo_colors);
        glBufferData(GL_ARRAY_BUFFER, N_DISCR*N_DISCR*sizeof(GLfloat), colors, GL_STREAM_DRAW);
        glDrawElements(GL_LINES_ADJACENCY, 4*(N_DISCR-1)*(N_DISCR-1), GL_UNSIGNED_INT, 0);
        // end = clock();

        // Print stuff
        cpu_time += (double)(end - begin) / CLOCKS_PER_SEC*1e3;
        printf("\rIter n°%5d, Time = %.6f [s] | Avg. CPU time per iteration = %1.3f [ms]", t, t*dt, cpu_time/t);
        fflush(stdout);

        // Save results to file
        // if (dt*t == 1e-3) {
        //     char str[80];
        //     sprintf(str, "imex2_%d_%e.txt", N_DISCR, dt);
        //     FILE *fp = fopen(str, "w+");
        //     for (int i = 0; i < N_DISCR*N_DISCR; i++)
        //         fprintf(fp, "%.20e\n", c[i]);
        //     fclose(fp);
        //     printf("\nOutput written to file\n");
        //     exit(EXIT_SUCCESS);
        // }
    }

    // Free memory
    glDeleteBuffers(1, &ebo);
    glDeleteBuffers(1, &vbo_pos);
    glDeleteBuffers(1, &vbo_colors);

    glDeleteVertexArrays(1, &vao);

    glfwDestroyWindow(window);
    glfwTerminate();
    free_shaders();
    free_solver();
    free(c);
    free(colors);
    free(elements);
    free(positions);

    return EXIT_SUCCESS;
}
